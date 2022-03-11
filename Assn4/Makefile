assignement : user1 user2

user1 : user1.c libass_socket.a
	cc -o user1 user1.c -L. -lass_socket -lpthread 

user2 : user2.c libass_socket.a
	cc -o user2 user2.c -L. -lass_socket -lpthread 

# This tells ar to create an archive (option c) and to insert the objects, replacing older files where needed (option r), (s-> silent?)
libass_socket.a: ass_socket.o
	ar -rcs libass_socket.a ass_socket.o

ass_socket.o : rsocket.c rsocket.h
	gcc -Wall -c rsocket.c -o ass_socket.o

clean:
	rm -rf user1 user2 libass_socket.a ass_socket.o *.gz
	clear

tarify:
	tar -cvf Assignment4_19CS30007_19CS10069.tar.gz rsocket.h rsocket.c user1.c user2.c Makefile documentation.txt README.txt