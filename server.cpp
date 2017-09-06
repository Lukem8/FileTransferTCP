#define _BSD_SOURCE
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <fstream>
#include <sys/unistd.h>
#include <sys/fcntl.h>
using namespace std;


#define BUFSIZE 2048
char buf[BUFSIZE+1];
string directory;




int sockfd;
// bool for signal polling
int flag = 0;

// value for number of client connections (used for output file naming)
int connectionCount = 0;

vector <thread> myvector;

void sig_handler(int signo)
{
  flag = 1;
}


//this function should read from the socket and write to file based on connectionCount
// also must check for 10 second requirement and 100 MiB requirement
void handleConnection(int childfd){
  signal(SIGQUIT, sig_handler);
  signal(SIGTERM, sig_handler);
  connectionCount++;
  string count = to_string(connectionCount);
  string fileName = directory + "/" + count + ".file";
  cout << fileName << endl;
  ofstream myfile;
  myfile.open(fileName, ios::binary | ios::out);

  fd_set readfds;
  struct timeval tv;
  int rv;
  FD_ZERO(&readfds);

  // add our descriptors to the set
  FD_SET(childfd, &readfds);

  tv.tv_sec = 10;
  tv.tv_usec = 0;
  rv = select(childfd+1, &readfds, NULL, NULL, &tv);
  if (rv == -1) {
      perror("select"); // error occurred in select()
  } else if (rv == 0) {
      printf("Timeout occurred!  No data after 10 seconds.\n");
      myfile.close ();
      ofstream ofs ("fileName", ios::out | ios::trunc); // clear contents
      string message = "ERROR: no data received from client for 10 seconds.";
      myfile.write(message.c_str(), message.length()+1);
      ofs.close ();
  }
  int b;
  // reading from socket into buffer, then write
  while((b = recv(childfd, buf, BUFSIZE+1, 0)) > 0){
    FD_ZERO(&readfds);
    // add our descriptors to the set
    FD_SET(childfd, &readfds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    rv = select(childfd+1, &readfds, NULL, NULL, &tv);
    if (rv == -1) {
        perror("select"); // error occurred in select()
    } else if (rv == 0) {
        printf("Timeout occurred!  No data after 10 seconds.\n");
        myfile.close ();
        ofstream ofs ("fileName", ios::out | ios::trunc); // clear contents
        string message = "ERROR: no data received from client for 10 seconds.";
        myfile.write(message.c_str(), message.length()+1);
        ofs.close ();
    }

    myfile.write(buf, b);

    bzero(buf, BUFSIZE);
    if(flag){
      break;
    }
  }
  cout << "after recv" << endl;
  myfile.flush();
}


int main(int argc, char** argv){

  int portNum = atoi(argv[1]);
  directory = argv[2];  // save the specified directory

  struct sockaddr_in addr;
  socklen_t addr_size = 0;
  int childSock;
  sockaddr_in sadr;

  sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);


  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
   cerr << "ERROR: setting up socket" << endl;
   return 1;
 }

  addr.sin_family = AF_INET ;
  addr.sin_port = htons(portNum);

  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  addr.sin_addr.s_addr = INADDR_ANY;

  if( bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1 ){
      cerr << "ERROR: binding to socket" << endl;
      exit(1);
  }
  if(listen(sockfd, 1) == -1 ){
      cerr << "ERROR: error listening on socket" << endl;
      exit(1);
  }


  struct addrinfo hints, *servinfo;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    cerr << "ERROR: unable to connect" << endl;
    exit(1);
}


  while(1){
      signal(SIGQUIT, sig_handler);
      signal(SIGTERM, sig_handler);
      signal (SIGINT, sig_handler);
      if (flag){
        for (int i = 0; i < connectionCount; i++){
          myvector[i].join();
        }
        close(sockfd);
        exit(0);
      }
      if ((childSock = accept(sockfd, (sockaddr*)&sadr, &addr_size)) != -1){
          cout << "-----Got connection" << endl;
          myvector.push_back(thread(handleConnection, childSock));
      }
  }

  // join threads
  for (int i = 0; i < connectionCount; i++){
    myvector[i].join();
  }
  close(sockfd);
}
