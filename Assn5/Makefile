run: tracer
	./tracer localhost

compile: mytraceroute_19CS10069_19CS30007.c
	sudo su
	cc -o tracer mytraceroute_19CS10069_19CS30007.c

client: my_udpclient.c
	cc -o client my_udpclient.c
	./client

ref: ref.c
	cc -o ref ref.c
	sudo ./ref www.google.com 2> error.log

ref_orig: ref_orig.c
	cc -o ref_orig ref_orig.c
	sudo ./ref_orig www.google.com

clean:
	rm -f client tracer ref ref_orig
	exit
