/*
  iPhone ver3.0

  - fft with bandpass
  - less delay
  - ssl encryption
  - udp hole punching

  [Build command]
  gcc iphone3.c -o iphone3 -lm -lssl -lcrypto

  [Usage]
  ./iphone2
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <complex.h>

#define N 1024
#define BUFLEN 512
#define NPACK 10
#define PORT 9930
#define SRV_IP "62.77.156.137"

int pid;
FILE *fp_play, *fp_rec;

void convert(long n, long min, long max, void * buf);
 
// A small struct to hold a UDP endpoint. We'll use this to hold each peer's endpoint.
struct peer {
  unsigned int host;
  unsigned int port;
};
 
// Just a function to kill the program when something goes wrong.
void die(char *s) {
  perror(s);
  exit(1);
}

void abort_handler(int signal) {
  exit(0);
}
 
int main(int argc, char* argv[]) {
  struct sockaddr_in si_other;
  int s;
  socklen_t slen = sizeof(si_other);
  char buf[BUFLEN];
  struct peer other;
 
  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    die("socket");
 
  // The server's endpoint data
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(PORT);
  if (inet_aton(SRV_IP, &si_other.sin_addr)==0)
    die("aton");
 
  if (sendto(s, "hi", 2, 0, (struct sockaddr*)(&si_other), slen)==-1)
    die("sendto");
 
  if (recvfrom(s, &other, sizeof(other), 0, (struct sockaddr*)(&si_other), &slen) == -1)
    die("recvfrom");
  printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

  si_other.sin_addr.s_addr = htonl(other.host);
  si_other.sin_port = htons(other.port);
  printf("add peer %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

  if ((pid = fork()) == 0) {
    // start play command
    char data_play[N];
    fp_play = popen("play -q -t raw -b 16 -c 2 -e s -r 44100 -", "w");
    if (fp_play == NULL) {
      perror("command");
      exit(1);
    }
    
    while (1) {
      // play
      int n = recvfrom(s, data_play, N, 0, (struct sockaddr*)(&si_other), &slen);
      if (n == -1) die("recvfrom");
      if (n == 0) break;
      fwrite(data_play, sizeof(data_play), 1, fp_play);
    }
    fclose(fp_play);
  } else if (pid == -1) {
    fprintf(stderr, "parent process failed");
    return(1);
  } else {
    // start rec command
      char data_rec[N];
      fp_rec = popen("rec -V1 -t raw -b 16 -c 2 -e s -r 44100 -", "r");
      if (fp_rec == NULL) {
        perror("command");
        exit(1);
      }

      while (1) {
        // rec
        size_t size = fread(data_rec, N, 1, fp_rec);
        if (size == 0) break;
        if (feof(fp_rec)) {
          perror("read");
          exit(1);
        }
        convert(N, 300, 3400, data_rec);
        sendto(s, data_rec, sizeof(data_rec), 0, (struct sockaddr*)(&si_other), slen);
      }
      fclose(fp_rec);
  }

  // Actually, we never reach this point...
  close(s);
  kill(pid, SIGKILL);
  return 0;
}

/******** ここより下はfft.cのコピー ********/

typedef short sample_t;

/* 標本(整数)を複素数へ変換 */
void sample_to_complex(sample_t * s, complex double * X, long n) {
  long i;
  for (i = 0; i < n; i++) X[i] = s[i];
}

/* 複素数を標本(整数)へ変換. 虚数部分は無視 */
void complex_to_sample(complex double * X, sample_t * s, long n) {
  long i;
  for (i = 0; i < n; i++) {
    s[i] = creal(X[i]);
  }
}

/* 高速(逆)フーリエ変換;
   w は1のn乗根.
   フーリエ変換の場合   偏角 -2 pi / n
   逆フーリエ変換の場合 偏角  2 pi / n
   xが入力でyが出力.
   xも破壊される
 */
void fft_r(complex double * x, complex double * y, long n, complex double w) {
  if (n == 1) {
    y[0] = x[0];
  } else {
    complex double W = 1.0; 
    long i;
    for (i = 0; i < n/2; i++) {
        y[i]   =   (x[i] + x[i+n/2]); /* 偶数行 */
        y[i+n/2] = W * (x[i] - x[i+n/2]); /* 奇数行 */
        W *= w;
    }
    fft_r(y,   x,   n/2, w * w);
    fft_r(y+n/2, x+n/2, n/2, w * w);
    for (i = 0; i < n/2; i++) {
      y[2*i]   = x[i];
      y[2*i+1] = x[i+n/2];
    }
  }
}

void fft(complex double * x, 
  complex double * y, 
  long n) {
  long i;
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) - 1.0j * sin(arg);
  fft_r(x, y, n, w);
  for (i = 0; i < n; i++) y[i] /= n;
}

void ifft(complex double * y, 
  complex double * x, 
  long n) {
  double arg = 2.0 * M_PI / n;
  complex double w = cos(arg) + 1.0j * sin(arg);
  fft_r(y, x, n, w);
}

void bandpass(long min, long max, complex double * Y, long n) {
  for (long i = 0; i < min; i ++) {
    Y[i] = 0;
  }
  for (long i = max; i < n; i ++) {
    Y[i] = 0;
  }
}

void convert(long n, long min, long max, void * buf) {
  complex double * X = calloc(sizeof(complex double), n);
  complex double * Y = calloc(sizeof(complex double), n);
  sample_to_complex(buf, X, n);
  fft(X, Y, n);
  bandpass(min, max, Y, n);
  ifft(Y, X, n);
  complex_to_sample(X, buf, n);
}
