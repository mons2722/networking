#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<time.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>

#define PORT "3490"
#define BACKLOG 10

void main()
{ int sockfd,newc;
  struct sockaddr_storage caddr;
  struct addrinfo hint,*res,*p;
  sockt_len sin_size;
  struct sigaction signal;
  int yes=1; //for sockopt()
  char s[INET6_ADDRSTRLEN];
  int status;//getaddrinfo() return type

  memset(&hint,0,sizeof(hint));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status=getaddrinfo(NULL,PORT,&hint,&res))!=0)
    { fprintf(stderr,"Error %s\n",gai_strerror(status));
	 exit(1);
    }

    for(p=res;p!=NULL;p=p->ai_next)
    { if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
	    {  perror("Server:Socket\n");
	       continue;}
      
      if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) 
      {perror("setsockopt");
       exit(1);
      }


      


