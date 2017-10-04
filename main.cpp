/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/shm.h>
#include <sys/msg.h>
#include <error.h>
#include <vector>
#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define MAXDATASIZE 1000
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void GetAddressInfo(struct addrinfo *&servinfo) {
    int rv;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        // return 1;
    }
}

void BindFirstSucc(struct addrinfo *servinfo, int &sockfd) {
    struct addrinfo *p;
    int yes = 1;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
}

void ListenOnSocket(int sockfd) {
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
}

void ReapDead() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}


void SendProcess(int new_fd)
{
    while (true) {
        std::string word;
        std::getline(std::cin, word);

        //sleep(1);
        if (send(new_fd, word.c_str(), word.length(), 0) == -1) {
            std::cout << "exited from the sending fork" << std::endl;
            exit(0);
        }

    }
}
void RecieveProcess(int new_fd,int childID)
{
    char *buf [MAXDATASIZE];
    while (true) {
        int numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0);


        if (numbytes == 0) {
            std::cout << "the user has exited" << std::endl;
            kill(childID, SIGTERM);
            exit(0);
            break;
            //
        }
        buf[numbytes] = '\0';
        std::cout << "client number:" << new_fd << " ";
        printf(" responded with: %s\n", buf);
    }
}
//this is for the message queue data
struct buffer {
    long mtype;
    struct message {
        std::string data;
        std::string name;
    } msg;
};

int main(void) {
    /*
      // creating a message queue
      key_t key = ftok("./main.cpp",'a');

      //0666 is -rw -rw -rw
      int msqid = msgget(key,0666|IPC_CREAT);
      if(msqid == -1){
          perror("creating message queue");
      }
      buffer * bufQ=new buffer;
      bufQ->mtype=1;
      bufQ->msg.data="test data";
      bufQ->msg.name="3bhady";
  */


    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];
    int ServerPID;

    GetAddressInfo(servinfo);
    // loop through all the results and bind to the first we can
    BindFirstSucc(servinfo, sockfd);
    ListenOnSocket(sockfd);
    ReapDead(); // reap all dead processes





    printf("server: waiting for connections...\n");
   // if(!(ServerPID=fork())) {
        //this code if for the listening server **Main Server**
        while (1) {
            sin_size = sizeof their_addr;
            new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
            if (new_fd == -1) {
                perror("accept");
                continue;
            }
            int ListenPID; //the server process id that communicates with a new client
            //also Sid is the process id for the listening fork which is the parent for
            if (!(ListenPID = fork())) {
                inet_ntop(their_addr.ss_family,
                          get_in_addr((struct sockaddr *) &their_addr),
                          s, sizeof s);
                printf("server: got connection from %s\n", s);

                int SendPID;
                if (!(SendPID = fork())) {
                 //the sending fork for the client
                    SendProcess(new_fd);
                } else {
                    //the listening fork for specific client
                    RecieveProcess(new_fd,SendPID);
                }
            }
            else
            {
                //this is the code that gets executed in the **Main server** process
                //which listens on the socket for new connections
            }
        }
    //}
 //   else //this code is for the chat manager
 //   {
    //ServerPID contains the process id listening for new connections

    //two vectors containing PIDs of the listening and sending processes
 //   std::vector<int>SendPIDs,ListenPIDs;



   // }
    return 0;
}