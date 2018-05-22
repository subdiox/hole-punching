all: server client
server: udp_server.c
	cc -o $@ $^
client: udp_client.c
	cc -o $@ $^
clean:
	rm server client
