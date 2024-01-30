#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/types.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<pthread.h>

#define bufsize 10000
#define PORT "443"
#define BACKLOG 10

//global definitions of ssl connections 
SSL *ssl1=NULL,*ssl2=NULL;

// Function to create SSL context
SSL_CTX *create_sslctx()
{  
  SSL_CTX *ctx;
  
   ctx = SSL_CTX_new(SSLv23_server_method());

    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Load the server certificate and private key
   if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
   {    ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
    {  ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
 
    return ctx;
}

void *get_in_addr(struct sockaddr *s) 
{
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);

    return &(((struct sockaddr_in6*)s)->sin6_addr);
}

//handle the messaging between clients
void *handle_client(void *arg)
{ 
 SSL *ssl=(SSL *)arg;
 char buf[bufsize];
 int cid=SSL_get_ex_data(ssl,0);
 int bytes;

 while(1)
 { 
  bytes=SSL_read(ssl,buf,bufsize);
  if(bytes>0)
  {buf[bytes]='\0';
   printf("Message from client %d:%s\n",cid,buf);

   //send message to other client
    SSL_write((cid==1)?ssl2:ssl1,buf,strlen(buf));
  }
 }
 SSL_free(ssl);
 pthread_exit(NULL);
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
    SSL *ssl;
    SSL_CTX *sslctx;
    pthread_t tid;
    
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
 
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    sslctx= create_sslctx();
    while(1)
    { 
    //accepting connections from client 
     sin_size= sizeof(caddr);
      newc=accept(sockfd,(struct sockaddr *)&caddr,&sin_size);
      if(newc==-1)
     {perror("Server:Accept\n");
       exit(1);
      }

      // Create SSL object
      ssl = SSL_new(sslctx);
     // Assign the connected socket to the SSL object
    SSL_set_fd(ssl, newc);
     //performing ssl handshake
    if (SSL_accept(ssl) <= 0)
    ERR_print_errors_fp(stderr);
    else{ if(ssl1==NULL)
	    { ssl1=ssl;
	    printf("\nConnected to Client 1");
	   SSL_set_ex_data(ssl,0,(void*)1);
	  pthread_create(&tid,NULL,handle_client,(void*)ssl);
            }
	    else if(ssl2==NULL)
	    {  ssl2=ssl;    
               printf("\nConnected to Client 2");
               SSL_set_ex_data(ssl,0,(void*)2);
               pthread_create(&tid,NULL,handle_client,(void*)ssl);}
    }
    }

    close(newc);
    SSL_shutdown(ssl);
   
    SSL_CTX_free(sslctx);
    close(sockfd);
}
