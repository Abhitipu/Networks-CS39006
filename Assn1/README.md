# Assignment 1
## TCP/UDP

# Directions to use the code  
1. Move to the TCP/UDP folder as per requirement and open two terminals.

## [a] Using Makefile
2. On the first one, run
 `make server`

3. On the second one, run
 `make`

## [b] Without using Makefile
2. On the first one, run
 `cc -o server tcpserver.c` or `cc -o server udpserver.c` <br/>
 Then run ``./server``

3. On the second one, run
 `cc -o client tcpclient.c` or `cc -o client udpclient.c` <br/>
 Then run ``./client <inputfilename>``

This will print the buffer from the server side as well as the client side. It then shows the results on the client side.
