run: tracer
	./tracer localhost

compile: mytraceroute_19CS10069_19CS30007.c
	sudo su
	cc -o tracer mytraceroute_19CS10069_19CS30007.c

client: my_udpclient.c
	cc -o client my_udpclient.c
	./client

clean:
	rm -f client tracer
	exit