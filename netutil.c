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

#include "netutil.h"

// sample handler for trap_signals()
int server_sockid = 0;
void server_killed (int n) { // will be called on INT or TERM signal
  fprintf (stderr, "Caught signal %d, exitting...\n", n);
  close (server_sockid); exit (0); 
}

// redirect bad signals to the user-supplied handler function
void trap_signals (void (*handle)(int)) {
  signal (SIGHUP,  handle);
  signal (SIGTERM, handle);
  signal (SIGINT,  handle);
  signal (SIGQUIT, handle);
  signal (SIGBUS,  handle);
  signal (SIGSEGV, handle);
  signal (SIGPIPE, handle);
#ifdef SIGPWR 
  signal (SIGPWR,  handle); // not defined on a Mac
#endif
  signal (SIGFPE,  handle);
  //signal (SIGABRT, handle);
}

// simple error-handling wrapper around network functions
int safe (char *comment, int result) {
  if (result == -1) {
    fprintf (stderr, "--------------------------------------------------\n");
    fprintf (stderr, "[%s] failed: [%d] ", comment, errno);  perror (""); 
    fprintf (stderr, "--------------------------------------------------\n");
    // assert (0); 
  }
  return result;
}

// convert host ("10.0.0.1" or "www.cnn.com") to integer value
unsigned long host2ip (char *host) {
  if (!host) return htonl (INADDR_ANY);
  unsigned long ip = inet_addr (host);
  if (ip == (unsigned long) -1) { // try to get from DNS;
    struct hostent *hp = gethostbyname (host);
    if (!hp) { herror (host); assert (0); }
    memcpy (&ip, hp->h_addr_list[0], sizeof (unsigned long));
    endhostent (); // free up system resources used for fetching
  }
  return ip;
}

// create a sockaddr structure for host:port
saddr_t *mkaddr (char *host, int port) {
  saddr_t *addr = calloc (1, sizeof (saddr_t));
  addr->sin_family = AF_INET;
  addr->sin_port = htons (port);
  addr->sin_addr.s_addr = host2ip (host); 
  return addr;
}

// create a socket, set LINGER and REUSE (in case we crash)
// optionally make it NON-BLOCK for accept and send/recv
// bind to port, listen, return sockid
int server_socket (int port, int nonblock) {
  struct linger linger = {1, 0}; // linger:true, time:0sec
  int flag = 1;
  saddr_t *server = mkaddr (NULL, port);
  int sockid = safe ("socket", socket (AF_INET, SOCK_STREAM, 0)); 
  if (nonblock) safe ("fcntl", fcntl (sockid, F_SETFL, O_NONBLOCK));
  safe ("setsockopt", setsockopt (sockid, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)));
  safe ("setsockopt", setsockopt (sockid, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)));
  //safe("setsockopt",setsockopt (sockid, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)));
  safe ("bind", bind (sockid, (paddr_t) server, sizeof (saddr_t))); 
  safe ("listen", listen (sockid, 100)); // queue up to 100 connects
  return sockid;
}

// create a socket, connect to host ("www.cnn.com:80"), return sockid
int client_socket (char *host) { 
  char *port = index (host, ':');
  *port++ = '\0'; // null-terminate host, point to port
  saddr_t *server = mkaddr (host, atoi(port));
  int sock = safe ("socket", socket (AF_INET, SOCK_STREAM, 0)); 
  int err = connect (sock, (paddr_t) server, sizeof (saddr_t));
  if (err == -1) {
    fprintf (stderr, "[connect] %s:%s failed: [%d] ", host, port, errno);
    perror (""); 
    close (sock);
    return -1;
  }
  return sock;
}

// create a server listening on a specified port
// invoke user-supplied handler for each accepted connection
// handler will be passed a client socket, cast as a (void*)
// handler must close client socket when done
void server_loop (int port, void* (*handle) (void*), int threaded) {
  server_sockid = server_socket (port, 0); // blocking socket
  trap_signals (server_killed); // close sockid on signals
  fprintf (stderr, "Listening on port %d %s\n", port, (threaded?"threaded":""));
  while (1) {
    int c_socket = safe ("accept", accept (server_sockid, NULL, NULL));
    void *client = (void*) (long) c_socket;
    if (threaded) {
      pthread_t t = 0;
      pthread_create (&t, NULL, handle, client);
      pthread_detach (t); // handle must close (client) !!!
    }
    else handle (client);
  }
}

// accept a connection, send data to the client, close connection
// return number of bytes sent, or -1 if no clients waiting
// if sockid is non-blocking, the call will return immediately
int accept_and_send (int sockid, void *data, int size) {
  if (sockid == -1) return -1; // invalid listening socket
  int client = accept (sockid, NULL, NULL);
  if (client == -1) return -1; // noone waiting to connect
  int sent = send (client, data, size, (MSG_NOSIGNAL|MSG_DONTWAIT));
  shutdown (client, SHUT_RDWR);
  close (client);
  return sent;
}

// same as above but connection is kept open for subsequent calls 
// -- useful for 'pushing' a data to the client (e.g. newsfeed)
int accept_send_hold (int sockid, char *message, int size) { // thread-unsafe: static
  static int client = -1; 
  if (sockid == -1) return -1;
  if (client == -1) client = accept (sockid, NULL, NULL);
  if (client == -1) return -1;
  int sent = send_message (client, message, size);
  if (sent != size) {close (client); client = -1;}
  return sent;
}

// accept a connection, read data from the client, close connection
// return number of bytes received, or -1 if no clients waiting
// if sockid is non-blocking, the call will return immediately,
// unless there is data to be read (recv is blocking)
int accept_and_recv (int sockid, void *buf, int size) {
  if (sockid == -1) return -1; // invalid listening socket
  int client = accept (sockid, NULL, NULL);
  if (client == -1) return -1; // noone waiting to connect
  int recvd = recv (client, buf, size, (MSG_NOSIGNAL));
  shutdown (client, SHUT_RDWR);
  close (client);
  return recvd;
}

int agetline (int fd, char *buf, char **b, int sz) {
  for (; *b < buf+sz-1; ++*b) {
    int n = read (fd, *b, 1); // try to read 1 byte
    if      (n == -1 && errno != EAGAIN) { perror("[readmsg] ERROR"); return 0; }
    else if (n == -1) { return 0; }
    else if (n ==  0) { warnx("agetline -> EOF");    return 0;}
    else if (n !=  1) { warnx("agetline -> WTF??");  return 0;}
    else if (**b == '\n') {
      **b = 0; // null-terminate
      *b = buf; // reset state
      warnx("agetline -> %s", buf);
      return 1; }
    //if (n != 1) return 0; // try again or error or end-of-file
  }
  warnx("agetline -> NOMEM"); 
  return 0;
}

// receive and return a set of bytes terminated by eom or EOF
char *receive_message (int sockid, char eom) {
  int N = 10000, i = 0;
  char *buf = malloc(N);
  while (1) {
    int n = read (sockid, buf+i, 1);
    if (n <= 0 || buf[i] == eom) {buf[i] = 0; break;} // end of file / message
    if (++i == N) buf = realloc (buf, N*=2); // end of buffer => make it bigger
  }
  return buf;
}

int send_message (int sockid, char *message, int size) {
  int sent, flags = MSG_NOSIGNAL | MSG_DONTWAIT, wait=1;
  while (size > 0) {
    sent = send (sockid, message, size, flags);
    if (sent == -1) { 
      if (errno == EAGAIN) usleep (wait*=2); 
      else perror ("send_message: "); 
      continue;
    } 
    if (sent < size) fprintf (stderr, "[send_message] sent %d < %d bytes, retrying\n", sent, size);
    message += sent; size -= sent;
  }
  return (size == 0);
}

char *extract_message (char *buf, char *eom, int *used) {
  char *end = strstr (buf, eom);       // try to find end-of-message marker
  if (!end) return NULL;               // did not get a complete message
  uint mlen = (end-buf) + strlen(eom); // full message length
  char *msg = malloc (mlen+1);         // strndup (buf, mlen) (absent in OSX)
  memcpy (msg, buf, mlen);             
  msg[mlen]=0;
  *used -= mlen;                       // buf will now have mlen fewer bytes
  memmove (buf, buf+mlen, *used);      // new buf = end ... buf+used
  return msg;
}

char *recv_message (int sockid, char *eom) { // thread-unsafe: static
  int get = 10000, got, flags = MSG_NOSIGNAL | MSG_DONTWAIT;
  static int N = 10000, used = 0;
  static char *buf = NULL;
  if (!buf) buf = malloc (N);
  while(1) { // receive as much data as we can get without blocking
    if (used+get > N) buf = realloc (buf, N *= 2); // buf big enough?
    got = recv (sockid, buf+used, get, flags); // try to get a chunk 
    if (got <= 0) break; // stop if no more data at the moment
    else used += got;
  }
  return extract_message (buf, eom, &used);
}

int sends (int socket, char *str) {
  int len = strlen (str);
  //int sent = send (socket, str, len, MSG_NOSIGNAL | MSG_DONTWAIT);
  //int sent = send (socket, str, len, MSG_MORE);
  int sent = send (socket, str, len, MSG_NOSIGNAL);
  //int sent = write (socket, str, len);
  if (sent != len) warn ("send ERROR %d", errno);
  fprintf (stderr, "--> UI: %s", str);
  return (sent == len) ? 0 : errno;
  //usleep(1000);
}

char *sgets (char *buf, int size, int sock) {
  int i=-1, flags = MSG_NOSIGNAL; // | MSG_DONTWAIT;
  while (++i < size-1) {
    int got = recv (sock, buf+i, 1, flags);
    if (got == -1) perror ("[sgets:recv] failed");
    if (got < 1 || buf[i] == '\n') break;
  }
  buf[i] = '\0';
  return (i>0) ? buf : NULL;
}

char *asgets (char *buf, int size, int sock, char **end) { // non-blocking 
  int flags = MSG_NOSIGNAL | MSG_DONTWAIT;
  for (; *end < buf+size-1; ++*end) {
    int got = recv (sock, *end, 1, flags);
    if (got == -1 && errno != EAGAIN) perror ("[asgets:recv] failed");
    if (got < 1) return NULL; // not enough data
    if (**end == '\n') break;
  }
  **end = '\0';
  return (*end = buf); // next call will start here
}

int sputs (char *buf, int sock) {
  int size = strlen(buf), flags = MSG_NOSIGNAL; // | MSG_DONTWAIT;
  int sent = send (sock, buf, size, flags);
  if (sent == -1) perror ("[sputs:send] failed");
  if (sent != size) fprintf (stderr, "ERROR: didn't send: %s\n", buf);
  return (sent == size);
}

int fdprintf (int fd, char *fmt, ...) {
  char buf [10000];
  va_list args;
  va_start (args, fmt);
  int size = vsprintf (buf, fmt, args);
  va_end (args);
  int sent = send (fd, buf, size, MSG_NOSIGNAL);
  if (sent == -1) perror ("[fdprintf:send] failed");
  else if (sent != size) warnx ("WARNING: fdprintf sent %d < %d bytes from: %s", sent, size, buf);
  return sent;
}

int seof (int fd) {
  char buf[10];
  int got = recv (fd, buf, 1, MSG_PEEK | MSG_DONTWAIT | MSG_NOSIGNAL);
  if (got == -1 && errno != EAGAIN) perror ("[seof:recv] failed");
  return (got == 0); // 0 bytes => orderly remote shutdown
}

void close_socket (int fd) {
  char buf[1000];
  if (shutdown (fd, SHUT_WR)) warn("shutdown [%d]", errno);
  while (0 < recv (fd, buf, 999, MSG_NOSIGNAL));
  usleep(100000); // otherwise chrome and safari get ERR_CONNECTION_RESET
  if (close (fd)) warn("close [%d]", errno);
}

int peek (int fd, char *buf, int sz, int ttl) {
  int got = 0, flags = MSG_PEEK | MSG_DONTWAIT | MSG_NOSIGNAL;
  while (got < sz && ttl-- > 0) {
    got = recv (fd, buf, sz, flags);
    //warnx("[%d] %d = peek(%d), %d retries left", fd, got, sz, ttl);
    if (got < sz) usleep (100);
  }
  return got;
}

int readall (int fd, char *buf, int sz, int ttl) {
  int got = 0, used = 0;//, flags = MSG_NOSIGNAL | MSG_WAITALL;
  while (1) {
    got = recv (fd, buf+used, sz-used, 0);
    if      (got > 0) used += got;
    else if (got == 0) break;
    else if (ttl-- > 0) usleep(1000);
    else break;
    //else if (got < 0 && ttl-- > 0) 
    //else if (errno == EAGAIN && ttl-- > 0) usleep(1000);
    //else warn ("[%d] read ERROR %d", fd, errno);
  }
  return used;
}

void socket_timeout (int fd, uint sec) {
  struct timeval tv;
  tv.tv_sec = sec;  /* 30 Secs Timeout */
  tv.tv_usec = 0;  // Not init'ing this can cause strange errors
  setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
}
