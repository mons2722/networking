#include<stdio.h>
#include<stdlib.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<errno.h>
#include<arpa/inet.h>
#include<string.h>

#define PORT "8888"
#define BUFSIZE 10000
#define host "127.0.0.1"

void *get_in_addr(struct sockaddr *s)
{
  if(s->sa_family==AF_INET)
    return &(((struct sockaddr_in*)s)->sin_addr);

  return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void send_req(int server,char method[], char data[])
{
  char http_req[BUFSIZE];
  
  //construct http request
  if (strcmp(method, "GET") == 0 || strcmp(method, "CONNECT") == 0) {
        sprintf(http_req,"%s HTTP/1.1\r\nHost: localhost\r\n\r\n", method);
    } 
  else if (strcmp(method, "POST") == 0) 
    { 
        sprintf(http_req,
		"%s HTTP/1.1\r\nHost: localhost\r\nContent-Length: %lu\r\n\r\n%s",
                 method,strlen(data), data);
    } else {
        fprintf(stderr, "Invalid HTTP method\n");
        close(server);
        exit(1);
    }

  send(server,http_req,strlen(http_req),0);
}

void get_req(int server)
{
  char method[100];
  
  char data[BUFSIZE];
  
  //take input of path,method and data to send request
  printf("Enter the HTTP method(POST/GET/CONNECT):");
  scanf("%s",method);

    if(strcmp(method,"POST")==0)
  {
    printf("Enter data for POST request:");
    fgets(data,sizeof(data),stdin);
    fgets(data,sizeof(data),stdin);
  }
  else
	  data[0]='\0';// for non-POST requests

  
  printf("HTTP client connecting to server.......\n");
 
  send_req(server,method,data);
}

void recv_req(int server)
{ 
 //receive and print server's response	
 char buf[BUFSIZE];
 recv(server,buf,BUFSIZE,0);
 printf("Server Response:\n%s\n",buf);
}

void main()
{
  int sockfd;
  struct addrinfo hint, *res, *p;
  char s[INET6_ADDRSTRLEN];
  int status;

     // Set up address information for the server
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, PORT, &hint, &res)) != 0) {
        fprintf(stderr, "Error %s\n", gai_strerror(status));
        exit(1);
    }

    // Iterate through the list of address structures to find a suitable one
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Socket");
            continue;
        }

        // Connect to the server
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Connect");
            continue;
        }
        break;
    }

    // Check if a suitable address structure was found
    if (p == NULL) {
        fprintf(stderr, "Client: Failed to connect\n");
        exit(1);
    }

    // Convert server address to a readable format
 inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    printf("Client: Connected to %s\n", s);

    // Free the address information
    freeaddrinfo(res);

    printf("HTTP Client:\n");
    
    while(1)
    { get_req(sockfd); 
      recv_req(sockfd);
    }

   close(sockfd);
}



