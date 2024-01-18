#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/types.h>

#define bufsize 10000
#define PORT "8888"//proxy server port
#define S_PORT "8080"
#define BACKLOG 10

// function to handle HTTP GET request
void getreq(int client)
{
  char *html_content=
	 "<html><body><h1>This is a simple html proxy server!</h1></body></html>";
  char resp[bufsize];

  sprintf(resp, "HTTP/1.1 200 OK\r\n"
                "Content-Length: %lu\r\n"
           "Content-Type: text/html\r\n\r\n%s",strlen(html_content),html_content);
 send(client,resp,bufsize,0);
}

// ffor(p=res;p!=NULL;p=p->ai_next)
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

    freeaddrinfo(res);// no more neededunction to handle HTTP POST request

void postreq(int client, char *p)
{
  printf("Recieved Message:\n");
  int n= strlen(p),count=1;

    while((count++)<=n)
  {  if(*p=='+')
	  { putchar(' ');
	  p++;
	  continue;
	  }
     if(*p=='%')
     {putchar('\n');
       p+=6;
       count+=5;
       continue;}
      putchar(*p++);
  }
  printf("\n");
  char resp[bufsize];
  char *html_data="<html><body><h1>Post request recieved!!</h1></body></html>";

  sprintf(resp,"HTTP/1.1 200 OK\r\n"
                "Content-Length: %lu\r\n"
                "Content-Type: text/html\r\n\r\n%s",strlen(html_data),html_data);

  send(client,resp,bufsize,0);
}

int create_remote(char host[],int port)
{
  int remote_socket=socket(AF_INET,SOCK_STREAM,0);

//function to handle http CONNECT request
void connect_req(int client,char *buf)
{
    char host[bufsize];
    int port;

    sscanf(buf,"CONNECT %s:%d",host,port);
    printf("Handling CONNECT method for %s:%d\n",host,port);

    char *success_response = "HTTP/1.1 200 OK\r\n\r\n";
    send(client, success_response, strlen(success_response), 0);

    //creates new remote socket
   int remote=create_remote(host,port);
}

// Function to forward data between sockets
void forward_data(int from_socket, int to_socket)
{
  char buffer[bufsize];
  ssize_t bytes;

  while((bytes=recv(from_socket,buffer,bufsize,0))>0)
       send(to_socket,buffer,bytes,0);
}

void handle_http_req(int client)//checks for the type of request
{
  char buf[bufsize];
  memset(buf,0,sizeof(buf));

  recv(client,buf,bufsize,0);
 
  if(strstr(buf,"GET")!=NULL)
        getreq(client);
  else if(strstr(buf,"POST")!=NULL)
	  postreq(client,buf);
  else if(strstr(buf,"CONNECT")!=NULL)
          connect_req(client,buf);
  else
       {
	 //handle other requests and send error message
        char *error="HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        send(client,error,sizeof(error),0);
       }
  }

void *get_in_addr(struct sockaddr *s)
{
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);

    return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void main()
{
    int sockfd, newc;
    struct sockaddr_storage caddr;
    struct addrinfo hint, *res, *p;
    socklen_t sin_size;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int status;

     //setup server address struct
    memset(&hint, 0, sizeof(hint));
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


