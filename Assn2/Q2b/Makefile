a.out:
	cc -o client dnsclient.c
	./client

tcp_client:
	cc -o client2 new_dnsclient.c
	./client2

new_tcp_client:
	cc -o client3 new_dnsclient.c
	./client3

udp_client:
	cc -o client4 dnsclient.c
	./client4

server: 
	cc -o server new_dnsserver.c
	./server

clean:
	rm -rf client* server
