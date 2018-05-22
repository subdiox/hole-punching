// UDP hole punching example, client code
// Base UDP code stolen from http://www.abc.se/~m6695/udp.html
// By Oscar Rodriguez
// This code is public domain, but you're a complete lunatic
// if you plan to use this code in any real program.
 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
 
#define BUFLEN 512
#define NPACK 10
#define PORT 9930
 
// This is our server's IP address. In case you're wondering, this one is an RFC 5737 address.
#define SRV_IP "123.57.55.85"
//#define SRV_IP "127.0.0.1"
 
// A small struct to hold a UDP endpoint. We'll use this to hold each peer's endpoint.
struct peer
{
    unsigned int host;
    unsigned int port;
};
 
// Just a function to kill the program when something goes wrong.
void diep(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char* argv[])
{
    struct sockaddr_in si_other;
    int s, i, f, j, k;
    socklen_t slen = sizeof(si_other);
    char buf[BUFLEN];
    struct peer other;
 
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        diep("socket");
 
    // The server's endpoint data
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    if (inet_aton(SRV_IP, &si_other.sin_addr)==0)
        diep("aton");
 
    if (sendto(s, "hi", 2, 0, (struct sockaddr*)(&si_other), slen)==-1)
        diep("sendto");
 
    if (recvfrom(s, &other, sizeof(other), 0, (struct sockaddr*)(&si_other), &slen)==-1)
        diep("recvfrom");
    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

    si_other.sin_addr.s_addr = htonl(other.host);
    si_other.sin_port = htons(other.port);
    printf("add peer %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

    for (k = 0; k < 10; k++)
    {
        if (sendto(s, "hi", 2, 0, (struct sockaddr*)(&si_other), slen)==-1)
            diep("sendto()");
        if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)(&si_other), &slen)==-1)
            diep("recvfrom()");
        printf("Received packet %s from %s:%d\n", buf, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
    }

    // Actually, we never reach this point...
    close(s);
    return 0;
}
