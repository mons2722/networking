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

int cl1=0,cl2=0;
 
// function to handle HTTP GET request
void getreq(int client,char *file)
{
  char resp[bufsize];
  char fname[bufsize];
 
  if(strlen(file)==1||strlen(file)==0)
  {
  char *html_content=
	  "<html><body><h1>This is a simple html server!</h1></body></html>";
  
  sprintf(resp, "HTTP/1.1 200 OK\r\n"
                "Content-Length: %lu\r\n"
		 "Access-Control-Allow-Origin: *\r\n"
               // Allow from any origin
               "Access-Control-Allow-Headers: *\r\n"
           "Content-Type: text/html\r\n\r\n%s",strlen(html_content),html_content);
  }

  else 
      {sprintf(fname,".%s",file);
       FILE *f=fopen(fname,"r");

       if(f!=NULL)
         { 
	fseek(f,0,SEEK_END);
        size_t	fsize=ftell(f);
	fseek(f,0,SEEK_SET); // go to file begining

	char *content=malloc(fsize);
	fread(content,1,fsize,f);
	fclose(f);
       
	sprintf(resp, "HTTP/1.1 200 OK\r\n"
                      "Content-Length: %lu\r\n"
                      "Content-Type: text/html\r\n\r\n%s", fsize,content);
	free(content);
	}
       else
       {  // File not found, return 404 error
        char *error= "<html><body><h1>404 Not Found</h1></body></html>";
        sprintf(resp, "HTTP/1.1 404 Not Found\r\n"
                      "Content-Length: %lu\r\n"
                      "Content-Type: text/html\r\n\r\n%s", strlen(error), error);
       }
      }
 send(client,resp,bufsize,0);
}

void forward(int to,char *mes)
{ 
  send(to,mes,bufsize,0);
}  
// function to handle HTTP POST request
void postreq(int client, char *p)
{ 
  printf("\nRecieved Message:\n");
  int n= strlen(p),count=1;
  
  if(client==cl1)
    forward(cl2,p);
  else
    forward(cl1,p);
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
  char *html_data="<h1>Post request received!!</h1>";
  
  sprintf(resp,"HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain\r\n"
               "Access-Control-Allow-Origin: *\r\n"
	       // Allow from any origin
	       "Access-Control-Allow-Headers: *\r\n"
               "\r\n%s",html_data);
  send(client,resp,strlen(resp),0);

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
    
    // Parse multipart data/

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

		char *end=strstr(filedata,"\n-------------");
	        if(!end)
		{ printf("File not supported!!\n");
		   break;}
	       else{	
		char file_content[bufsize];
		
                strncpy(file_content,filedata,end-filedata);
		file_content[end-filedata]='\0';
        	 printf("File Data:\n%s",file_content);
	       }
           }     
	    }
	    
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

   send(client,resp,bufsize,0);

   }

//function to handle http CONNECT request
void connect_req(int client)
{ 
    char *success_response = "HTTP/1.1 200 Connection established\r\n\r\n";
     send(client,success_response,bufsize,0);

}

void handle_http_req(int client)//checks for the type of request
{
  char buf[bufsize];
  memset(buf,0,sizeof(buf));
 
  recv(client,buf,bufsize,0);
//  printf("%s\n",buf);
   
  if(strstr(buf,"GET")!=NULL)
  { 	  char filename[100];
	  sscanf(buf,"GET %s",filename);
	  getreq(client,filename);
  }
  else if(strstr(buf,"POST")!=NULL)
  {  // check if the data is multipart/form-data   
    char *type;
     if(type = strstr(buf,"Content-Type: multipart/form-data"))
     {     multipart(client,buf);}
     else {
	     // find start of post_data
    char *start=strstr(buf,"\r\n\r\n");
    start+=strlen("\r\n\r\n");
    // move to start of actual data
    
    postreq(client,start);
  }
  }
  else if(strstr(buf,"CONNECT")!=NULL)
          connect_req(client);
  else
       { //handle other requests and send error message
        char *error="HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        send(client,error,bufsize,0);
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
    {
	    if((sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol))==-1)
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
   while(1)
   {
      sin_size= sizeof(caddr);
      newc=accept(sockfd,(struct sockaddr *)&caddr,&sin_size);
      if(newc==-1)
     {perror("Server:Accept\n");
       exit(1);
      }
   
      if(cl1==0)
      {
        cl1=newc;
       printf("\nConnected to Client 1");
       
      }
      else if(cl2==0)
      {cl2=newc;
       printf("\nConnected to Client 2");
       }
     
      handle_http_req(newc);
              }
  
   close(newc);
   close(cl1);
   close(cl2);
   close(sockfd);
}

