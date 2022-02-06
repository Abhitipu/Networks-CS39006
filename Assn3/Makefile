a.out: ftpC.c
	cc -o client ftpC.c
	./client

server: ftpS.c
	cc -o server ftpS.c
	./server

clean:
	rm -rf client server test temp.txt check.txt check.c temp/*
