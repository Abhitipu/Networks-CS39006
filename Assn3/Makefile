client: ftpC.c
	cc -o client ftpC.c
	./client

server: ftpS.c
	cc -o server ftpS.c
	./server

clean:
	rm -rf client server test temp.txt check.txt check.c temp/*

zipify:
	rm -rf *.zip
	zip Assignment3_19CS10069_19CS30007.zip Makefile user.txt ftpC.c ftpS.c