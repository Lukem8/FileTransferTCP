#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
using namespace std;



#define BUFSIZE 1024


int main(int argc, char** argv){
  char* host_name = argv[1];
  int portNum = atoi(argv[2]);
  ifstream file;
  file.open(argv[3] , ios::binary);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serveraddr;
  struct hostent *server;

  server = gethostbyname(host_name);
  if (server == NULL) {
      cerr << "ERROR: no such host" << endl;
      exit(1);
  }

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portNum);


  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(host_name, argv[2], &hints, &servinfo)) != 0) {
    cerr << "ERROR: unable to connect" << endl;
    exit(1);
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
          cerr << "ERROR: unable to connect" << endl;
          continue;
      }
      break;
  }

  int rt;
  int flags = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

  rt = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  struct timeval tv;
  fd_set watchFds;
  FD_ZERO(&watchFds);

  FD_SET(sockfd, &watchFds);
  if (rt < 0 && errno == EINPROGRESS){
    while(1){

    int fds = 0;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if ((fds = select(sockfd + 1, NULL, &watchFds, NULL, &tv)) == -1) {
      perror("select");
      exit(2);
    }
    if (fds == 0) {
      cerr << "ERROR: no data is received for 10 seconds!" << endl;
      exit(2);
    }
    if (fds == 1) {
      int so_error;
      socklen_t len = sizeof so_error;
      getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if (FD_ISSET(sockfd, &watchFds))
        break;
    }
  }
 }

  char buf[BUFSIZE+1];


  bzero(buf, BUFSIZE+1);
  long total = 0;

  FD_ZERO(&watchFds);
  FD_SET(sockfd, &watchFds);
  while(file){
    file.read(buf, BUFSIZE);
    streamsize bytes = file.gcount();
    total+= bytes;

    // do another select to check if sockfd is writable

    int fds = 0;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if ((fds = select(sockfd + 1, NULL, &watchFds, NULL, &tv)) == -1) {
      perror("select");
      exit(2);
    }
    if (fds == 0) {
      cerr << "ERROR: no data is received for 10 seconds!" << endl;
      exit(2);
    }

    send(sockfd, buf, bytes, 0);

    bzero(buf, BUFSIZE+1);
  }

  close(sockfd);
  file.close();
  return 0;
}
