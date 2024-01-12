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
#include<string.h>

#define PORT "3490"
#define MAX 100

void *get_in_addr(struct sockaddr *s)
{
  if(s->sa_family==AF_INET)
    return &(((struct sockaddr_in*)s)->sin_addr);

  return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void main(int argc, char *argv[])
{ 
  int sockfd,bytes;
  struct addrinfo hint,*res,*p;
  char buf[MAX];
  char s[INET6_ADDRSTRLEN];
  int status;

  if(argc!=2)
  { fprintf(stderr,"client:Hostname not provided\n");
    exit(1);
  }

 memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

    if((status=getaddrinfo(NULL,PORT,&hint,&res))!=0)
    { fprintf(stderr,"Error %s\n",gai_strerror(status));
         exit(1);
    }

    for(p=res;p!=NULL;p=p->ai_next)
    { if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
            {  perror("Client:Socket\n");
               continue;}

     if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1)
    { close(sockfd);
      perror("Connect\n");
      continue;
    }
   break;
    }

    if(p==NULL)
    { fprintf(stderr,"Client:Failed to connect \n");
      exit(1);
    }

   inet_ntop(p->ai_family,get_in_addr((struct sockaddr*)p->ai_addr),s,sizeof(s));
   printf("Client:Connecting to %s\n",s);

   freeaddrinfo(res);
   
  if((bytes=recv(sockfd,buf,MAX-1,0))==-1)
  {   perror("Client:recv");
    exit(1);
  }
  buf[bytes]='\0';
 :q
	 printf("Client:Recieved %s\n",buf);
  close(sockfd);
} 
