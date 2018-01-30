/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
  This file is part of YARI.
  
  YARI is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  YARI is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.
  
  You should have received a copy of the GNU General Public License
  along with YARI. If not, see <http://www.gnu.org/licenses/>.
  
*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdarg.h>

#ifndef NETUTIL
#define NETUTIL

// solaris has another mechanism for this
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

typedef struct sockaddr_in saddr_t;
typedef struct sockaddr* paddr_t;

//----------------------------------------------------------------------
//--- network wrappers for common tasks --------------------------------
//----------------------------------------------------------------------
// Example:
//   char *reply = "404 Page Not Found\n";
//   int srv = server_socket (80, 1);
//   while (1) accept_and_send (srv, reply, strlen(reply));


// simple error-handling wrapper around network functions
int safe (char *comment, int result) ;

// redirect bad signals to the user-supplied handler function
void trap_signals (void (*handle)(int)) ;

// convert host ("10.0.0.1" or "www.cnn.com") to integer value
unsigned long host2ip (char *host) ;

// create a sockaddr structure for host:port
saddr_t *mkaddr (char *host, int port) ;

// create a socket, set LINGER and REUSE (in case we crash)
// optionally make it NON-BLOCK for accept and send/recv
// bind to port, listen, return sockid
int server_socket (int port, int nonblock) ;

// create a socket, connect to host ("www.cnn.com:80"), return sockid
int client_socket (char *host) ;

// create a server listening on port
// invoke user-supplied handler for each accepted connection
// handler will be passed a client socket, must close it when done
void server_loop (int port, void* (*handle) (void*), int threaded) ;

// accept a connection, send data to the client, close connection
// return number of bytes sent, or -1 if no clients waiting
// if sockid is non-blocking, the call will return immediately
int accept_and_send (int sockid, void *data, int size) ;

// same as above but connection is kept open for subsequent calls 
// -- useful for 'pushing' a data to the client (e.g. newsfeed)
int accept_send_hold (int sockid, char *data, int size) ;

// accept a connection, read data from the client, close connection
// return number of bytes received, or -1 if no clients waiting
// if sockid is non-blocking, the call will return immediately,
// unless there is data to be read (recv is blocking)
int accept_and_recv (int sockid, void *buf, int size) ;

// receive and return a set of bytes terminated by eom or EOF
char *receive_message (int sockid, char eom) ;

// non-blocking, returns 1 if entire message sent, 0 otherwise
int send_message (int sockid, char *message, int size) ;

// non-blocking, receive and return a message terminated by 'eom'
// returns NULL if not complete message is ready
char *recv_message (int sockid, char *eom) ;

int sputs (char *buf, int sock) ;
char *sgets (char *buf, int size, int sock) ;
char *asgets (char *buf, int size, int sock, char **end) ;

int agetline (int fd, char *buf, char **b, int sz) ;

int sends (int socket, char *str) ;
int seof (int socket) ;
int fdprintf (int fd, char *fmt, ...) ;
void close_socket (int fd) ;

int peek (int fd, char *buf, int sz, int ttl) ;
int readall (int fd, char *buf, int sz, int ttl) ;

// maximum amount of time read or recv will wait for data
void socket_timeout (int fd, uint sec) ;

#endif
