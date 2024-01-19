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
#define S_PORT "8080"// server port
#define BACKLOG 10
#define host "127.0.0.1" // server hostname

// Function to forward data between sockets
void forward_data(int from_socket, int to_socket)
{
  char buffer[bufsize];
  ssize_t bytes;

  while((bytes=recv(from_socket,buffer,bufsize,0))>0)
       send(to_socket,buffer,bytes,0);
}

// function to handle HTTP GET and POST request
void get_or_post_req(int client,char *buf, int remote)
{
//forward data to server
 send(remote,buf,bufsize,0);
 //recieve respone from server and send to client
 forward_data(remote,client);
}

void *get_in_addr(struct sockaddr *s)
{
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);

    return &(((struct sockaddr_in6*)s)->sin6_addr);
}

int create_remote()
{
  int sockfd;//socket for end server
  struct addrinfo hint, *res, *p;
  char s[INET6_ADDRSTRLEN];
  int status;

   // Set up address information for the server
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;

     if ((status = getaddrinfo(host,S_PORT, &hint, &res)) != 0) 
     
     {
        fprintf(stderr, "Error %s\n", gai_strerror(status));
        exit(1);
    }

     // Iterate through the list of address structures to find a suitable one
    for (p = res; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
	{
            perror("Socket");
            continue;
        }
     
        // Connect to the server
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
	{
            close(sockfd);
            perror("Connect");
            continue;
        }
        break;
    }

    // Check if a suitable address structure was found
    if (p == NULL) {
        fprintf(stderr, "Proxy server: Failed to connect\n");
        exit(1);
    }
	
	printf ("$$\n");
    // Convert server address to a readable format
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));


    // Free the address information
    freeaddrinfo(res);

  return sockfd;
}
  


//function to handle http CONNECT request
void connect_req(int client,char *buf,int remote)
{
    printf("Handling CONNECT method for %s:%s\n",host,S_PORT);
    
     // Perform bidirectional forwarding between the client and the remote server
        fd_set read_fds;
        int max_fd = (client > remote) ? client : remote;

        while (1)
       	{
            FD_ZERO(&read_fds);
            FD_SET(client, &read_fds);
            FD_SET(remote, &read_fds);

            // Use select for multiplexing I/O
            if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) 
	    {
                perror("Error in select");
                break;
            }

            // Forward data from client to remote server
            if (FD_ISSET(client, &read_fds)) 
	    {
                forward_data(client, remote);
            }

            // Forward data from remote server to client
            if (FD_ISSET(remote, &read_fds)) 
	    {
                forward_data(remote, client);
            }
        }
}



void handle_http_req(int client,int remote)//checks for the type of request
{
  char buf[bufsize];
  memset(buf,0,sizeof(buf));

  recv(client,buf,bufsize,0);
  printf("%s\n",buf);
 
  if((strstr(buf,"GET")!=NULL)||(strstr(buf,"POST")!=NULL))
        get_or_post_req(client,buf,remote);
  else if(strstr(buf,"CONNECT")!=NULL)
          connect_req(client,buf,remote);
  else
       {
	 //handle other requests and send error message
        char *error="HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        send(client,error,sizeof(error),0);
       }
  }

void main()
{   int sockfd, newc;
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
 
    if(p==NULL)
    { fprintf(stderr,"Server failed to bind \n");
      exit(1);
    }

    if(listen(sockfd,BACKLOG)==-1)
    { perror("Server:listen\n");
      exit(1);
    }
    
    printf("Proxy Server:Waiting for connections on port %s.....\n",PORT);

      sin_size= sizeof(caddr);
      newc=accept(sockfd,(struct sockaddr *)&caddr,&sin_size);
      if(newc==-1)
      {perror("Server:Accept\n");
       exit(1);
      }
     
    inet_ntop(caddr.ss_family, get_in_addr((struct sockaddr*)&caddr), s, sizeof(s));
    printf("Server connected to %s\n", s);
    
    int remote=create_remote();

    while(1)
    { 
      handle_http_req(newc,remote);   
      }
   close(remote); 
   close(sockfd);
   close(newc);
  
}

