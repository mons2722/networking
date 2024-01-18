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
#define PORT "8080"
#define BACKLOG 10

// function to handle HTTP GET request
void getreq(int client)
{
  char *html_content=
	  "<html><body><h1>This is a simple html server!</h1></body></html>";
  char resp[bufsize];

  sprintf(resp, "HTTP/1.1 200 OK\r\n"
                "Content-Length: %lu\r\n"
                "Content-Type: text/html\r\n\r\n%s",strlen(html_content),html_content);
 send(client,resp,bufsize,0);
}

// function to handle HTTP POST request
void postreq(int client, char p[])
{  printf("@1");
  printf("%s\n",p);
  
  char resp[bufsize];
  char *html_data="<html><body><h1>Post request recieved!!</h1></body></html>";
  
  sprintf(resp,"HTTP/1.1 200 OK\r\n"
                "Content-Length: %lu\r\n"
                "Content-Type: text/html\r\n\r\n%s",strlen(html_data),html_data);

  send(client,resp,bufsize,0);
}

void multipart(int client,char *data)
{ 
   // Extract boundary from content-type header
    char *bound_start = strstr(data, "boundary=");
  
    if (!bound_start) {
        // Invalid request
        return;
    }
    // Move to the actual boundary value
    char bound[100];
    char *bound_end=strchr(bound_start,'\n');
   
    sscanf(bound_start,"boundary=%s\n",bound);
    // Parse multipart data

   char *part=strstr(bound_end,bound);
    while(part)
    {  printf("\n");
       char *part_end= strstr(part,bound);
	    
       if(!part_end)
          break;
	      
	       // Extract field name and filename from content-disposition
            char *name_start = strstr(part,"name=\"") + 6;
	    if(name_start)
	    {
            char *name_end = strchr(name_start, '\"');
            char field_name[256];
            strncpy(field_name, name_start, name_end - name_start);
	     field_name[name_end-name_start]='\0';
	    char *line =strchr(name_end,'\n');
	    if((line-name_end)>2)
	    {    
            char *filename_start = strstr(part, "filename=\"");
            if (filename_start) {
                filename_start += 10;
                char *filename_end = strchr(filename_start, '\"');
                char filename[256];
                strncpy(filename, filename_start, filename_end - filename_start);
                
	        char *content=strstr(filename_end,"Content-Type:");
	        char *filedata=strchr(content,'\n');
	        filedata+=2;
	        	
                printf("Field Name:%s\n", field_name);
                printf("File Name:%s\n", filename);
		printf("File Data:");
                while(*filedata!='-')
		       printf("%c",*filedata++);	
                 } }
	    
	    else {
                      printf("Field Name:%s\n", field_name);
	// Extract and print the value (excluding leading newline characters)
              char *value_start = strchr(name_end, '\n');
              if (value_start)
	      {
              value_start+=3; 

        printf("Field value:");
         while(*value_start!='-')
	      printf("%c",*value_start++);	 
   	      }
           }
	    }
	    
	  part=strstr(part+1,bound);

	  if(*(part+strlen(bound))=='-')
		    break;
          }
   char resp[bufsize];
   char *html_data = "<html><body><h1>Post request received!!</h1></body></html>";

   sprintf(resp, "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %lu\r\n"
               "Content-Type: text/html\r\n\r\n%s", strlen(html_data), html_data);

    send(client, resp, bufsize, 0);
}

//function to handle http CONNECT request
void connect_req(int client)
{ 
    char *success_response = "HTTP/1.1 200 Connection established\r\n\r\n";
    send(client, success_response, strlen(success_response), 0);
}

void handle_http_req(int client)//checks for the type of request
{
  char buf[bufsize];
  memset(buf,0,sizeof(buf));

  recv(client,buf,bufsize,0);
  // printf("%s\n",buf); 
  if(strstr(buf,"GET")!=NULL)
        getreq(client);
  else if(strstr(buf,"POST")!=NULL)
  {  // check if the data is multipart/form-data   
     char *type;
     if(type = strstr(buf,"Content-Type: multipart/form-data"))
     {     multipart(client,buf);}
     else {
	     // find start of post_data
    char *start=strstr(buf,"\r\n\r\n");
    start+=4; // move to start of actual data
    
    postreq(client,start);
  }
  }
  else if(strstr(buf,"CONNECT")!=NULL)
          connect_req(client);
  else
       { //handle other requests and send error message
        char *error="HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        send(client,error,sizeof(error),0);
       }
}

void *get_in_addr(struct sockaddr *s) {
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);

    return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void main() {
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

    if(p==NULL)
    { fprintf(stderr,"Server failed to bind \n");
      exit(1);
    }

    if(listen(sockfd,BACKLOG)==-1)
    { perror("Server:listen\n");
      exit(1);
    }
    
    printf("Server:Waiting for connections on port %s.....\n",PORT);

      sin_size= sizeof(caddr);
      newc=accept(sockfd,(struct sockaddr *)&caddr,&sin_size);
      if(newc==-1)
      {perror("Server:Accept\n");
       exit(1);
      }

    inet_ntop(caddr.ss_family, get_in_addr((struct sockaddr*)&caddr), s, sizeof(s));
    printf("Server connected to %s\n", s);
    
    while(1)
    { handle_http_req(newc);
      //infinite loop to handle requests
     }

   close(sockfd);
   close(newc);
}
