#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 2048

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>SelectWeb</title>\r\n"
"<style>body {background-color: #FFFF00 }</style><head>\r\n"
"<body><center><h1>Select Web Server</h1><br>\r\n"
"<img src=\"corgi.jpg\"></center></body></html>\r\n";

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int parentfd; /* parent socket */
  int childfd; /* child socket */
  int portno = 8080; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  char buf[BUFSIZE]; /* message buffer */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  int connectcnt; /* number of connection requests */
  int notdone;
  fd_set readfds;
  int fdimg;

  /* 
   * socket: create the parent socket 
   */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));

  /* this is an Internet address */
  serveraddr.sin_family = AF_INET;

  /* let the system figure out our IP address */
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* this is the port we will listen on */
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * listen: make this socket ready to accept connection requests 
   */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");


  /* initialize some things for the main loop */
  clientlen = sizeof(clientaddr);
  notdone = 1;
  connectcnt = 0;
  printf("server> ");
  fflush(stdout);

  /* 
   * main loop: wait for connection request or stdin command.
   *
   * If connection request, then echo input line 
   * and close connection. 
   * If command, then process command.
   */
  while (notdone) {

    /* 
     * select: Has the user typed something to stdin or 
     * has a connection request arrived?
     */
    FD_ZERO(&readfds);          /* initialize the fd set */
    FD_SET(parentfd, &readfds); /* add socket fd */
    FD_SET(0, &readfds);        /* add stdin fd (0) */
    if (select(parentfd+1, &readfds, 0, 0, 0) < 0) {
      error("ERROR in select");
    }

    /* if the user has entered a command, process it */
    if (FD_ISSET(0, &readfds)) {
      fgets(buf, BUFSIZE, stdin);
      switch (buf[0]) {
      case 'c': /* print the connection cnt */
	printf("Received %d connection requests so far.\n", connectcnt);
	printf("server> ");
	fflush(stdout);
	break;
      case 'q': /* terminate the server */
	notdone = 0;
	break;
      default: /* bad input */
	printf("ERROR: unknown command\n");
	printf("server> ");
	fflush(stdout);
      }
    }    

    /* if a connection request has arrived, process it */
    if (FD_ISSET(parentfd, &readfds)) {
      /* 
       * accept: wait for a connection request 
       */
      childfd = accept(parentfd, 
		       (struct sockaddr *) &clientaddr, &clientlen);
      if (childfd < 0) 
	error("ERROR on accept");
      connectcnt++;
      
      /* 
       * read: read input string from the client
       */
      bzero(buf, BUFSIZE);

	read(childfd, buf, 2047);
	//printf("%s\n", buf);
		
	if(!strncmp(buf, "GET /favicon.ico", 16))
	{
		fdimg = open("favicon,ico", O_RDONLY);
		sendfile(childfd, fdimg, NULL, 6000);
		close(fdimg);
	}
	else if(!strncmp(buf, "GET /corgi.jpg", 14))
	{
		fdimg = open("corgi.jpg", O_RDONLY);
		sendfile(childfd, fdimg, NULL, 122000);
		close(fdimg);
	}
	else
		write(childfd, webpage, sizeof(webpage) -1);
	close(childfd);

    }
  }

  /* clean up */
  printf("Terminating server.\n");
  close(parentfd);
  return 0;
}
