Instructions:

1. Open two terminals
2. You can change DROP_PROBABILITY in rsocket.h
3. Run command : make
4. In one terminal run : ./user2
5. In the other one, run : ./user1
6. After completion, run: make clean

Files involved

1. rsocket.h : Header file for r_socket library
2. rsocket.c : Implementation for functions in rsocket
3. user1.c   : The sender
4. user2.c   : The receiver
5. Makefile  : Associated makefile
