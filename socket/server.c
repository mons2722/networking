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
#define BACKLOG 10
#define MAXLEN 100

void *get_in_addr(struct sockaddr *s)
{ 
  if(s->sa_family==AF_INET)
    return &(((struct sockaddr_in*)s)->sin_addr);

  return &(((struct sockaddr_in6*)s)->sin6_addr); 
}

struct tm* recmess(char *mess, int newc)
{ time_t cur;
  struct tm* t;
  //getting current time 
  time(&cur);
  t=localtime(&cur);

  recv(newc,mess,MAXLEN,0);
  printf("Client:(%02d:%02d:%02d) %s\n",t->tm_hour,t->tm_min,t->tm_sec,mess);

  return t;
}

void sendmess(char *mess,int newc, struct tm* t)
{
  char messtime[MAXLEN];
  
  sprintf(messtime,"(%02d:%02d:%02d) %s",t->tm_hour,t->tm_min,t->tm_sec,mess);
  printf("Server:%s",messtime);
  send(newc,messtime,MAXLEN,0);
}
 
void main()
{ int sockfd,newc;
  struct sockaddr_storage caddr;
  struct addrinfo hint,*res,*p;
  socklen_t sin_size;
  int yes=1; //for sockopt()
  char s[INET6_ADDRSTRLEN];
  int status;//getaddrinfo() return type

   memset(&hint,0,sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

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

    if(bind(sockfd,p->ai_addr,p->ai_addrlen)==-1)
    { perror("Bind\n");
      close(sockfd);
      continue;
    }

    break;//break if binding successful
    }

    freeaddrinfo(res);// no more needed

    if(p==NULL)
    { fprintf(stderr,"Server failed to bind \n");
      exit(1);
    }

    if(listen(sockfd,BACKLOG)==-1)
    { perror("Server:listen\n");
      exit(1);
    }
    
    printf("Server:Waiting for connections.....\n");

      sin_size= sizeof(caddr);
      newc=accept(sockfd,(struct sockaddr *)&caddr,&sin_size);
      if(newc==-1)
      {perror("Server:Accept\n");
       exit(1);
      }
     

  inet_ntop(caddr.ss_family,get_in_addr((struct sockaddr*)&caddr),s,sizeof(s));
  printf("Server connected to %s\n",s);
  
  printf("Chat:\n");
  char mess[MAXLEN];
  memset(mess,0,sizeof(mess)); 
  struct tm* t; 
  while(1)
  { t=recmess(mess,newc);
    sleep(5);
	printf("$");    
    sendmess(mess,newc,t);	  
  }
  
  printf("Connection Closed!!\n");  
  close(sockfd);
  close(newc);
}

