#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/types.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include<pthread.h>

#define bufsize 10000
#define PORT "8080"
#define BACKLOG 10
#define MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"  

int cl[10],num=0;

// calculate accept_key for websocket
void get_accept_key(char *sec, char *client_key)
{ 
  char combined[100];
  strcpy(combined,sec);
  
  strcat(combined,MAGIC);
  
    // Calculate the SHA1 hash of the combined key
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)combined,strlen(combined),sha1_hash);
    
     // Base64 encode the SHA1 hash
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);

    BIO_write(b64, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    strcpy(client_key,bptr->data);

    // Remove trailing newline character
    size_t len = strlen(client_key);
    if (len > 0 && client_key[len - 1] == '\n') {
        client_key[len - 1] = '\0';
    }

    // Clean up BIO and base64 encoder
    BIO_free_all(b64);
}

// Function to handle WebSocket handshake
void websocket_handshake(int client, char *request)
{
char *key_start=strstr(request,"Sec-WebSocket-Key:")+strlen("Sec-WebSocket-Key: ");
char sec_key[100],acc_key[100];
sscanf(key_start,"%s\r\n",sec_key);

get_accept_key(sec_key,acc_key);

char response[bufsize];
    sprintf(response, 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
	"Sec-WebSocket-Accept: %s\r\n\r\n",acc_key);
    
    send(client, response, strlen(response), 0);
}

// Function to handle WebSocket data
void *handle_client(void *arg)
{ 
  int client=*((int *)arg);
  char buf[bufsize];

  while(1)
  { 
    ssize_t bytes=recv(client,buf,sizeof(buf),0);
    buf[bytes]='\0';
 //   printf("%s\n",buf);
    // check if it is in text format or not framented
    if((buf[0]&0x0F)==0x01 && (buf[0]&0x80)==0x80)
    {
    size_t payload_len=(buf[1]&0x7F);//getting the payload length
   
    int mask_flag = (buf[1] & 0x80) == 0x80;

    int payload_offset = 2 + (mask_flag ? 4 : 0); 
    // If mask is applied, the offset is 6, otherwise 2


    char payload[bufsize];
    memcpy(payload,buf+payload_offset,payload_len);
    payload[payload_len]='\0';
     // If masking is applied, unmask the payload data
    if (mask_flag) {
        // Extract the masking key
        char mask_key[4];
        memcpy(mask_key, buf + 2, 4);

        // Unmask the payload data
        for (size_t i = 0; i < payload_len; ++i) {
            payload[i] ^= mask_key[i % 4];
        }
    }

    printf("Message Received: %s",payload);
    }
    for(int i=0;i<num;i++)
    { if(cl[i]!=client)
	 send(cl[i],buf,bytes,0);
    }
  }
pthread_exit(NULL);  
}

void *get_in_addr(struct sockaddr *s) {
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);
    else  
        return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void main() {
    int sockfd,client;
    struct sockaddr_storage caddr;
    struct addrinfo hint, *res, *p;
    socklen_t sin_size;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int status;
    char buf[bufsize];
    
    // Setup server address struct
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hint, &res)) != 0) {
        fprintf(stderr, "Error %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Server:Socket\n");
            continue;
        }
      
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Bind\n");
            close(sockfd);
            continue;
        }

        break; // Break if binding successful
    }

    freeaddrinfo(res); // No more needed

    if (p == NULL) {
        fprintf(stderr, "Server failed to bind \n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("Server:listen\n");
        exit(1);
    }
    
    printf("Server:Waiting for connections on port %s.....\n", PORT);
    
    // Connect clients
    while(1)
    {    sin_size = sizeof(caddr); 
        client= accept(sockfd, (struct sockaddr *)&caddr, &sin_size); 
        recv(client, buf, bufsize, 0);
	websocket_handshake(client, buf);
	printf("Connected to client: %d\n",++num);
	pthread_t thread_id;
	 if (pthread_create(&thread_id, NULL, handle_client, &client) != 0) 
	 {
            perror("Thread creation failed");
            close(client);
            continue;
        }
	 pthread_detach(thread_id);
}
for(int i=0;i<num;i++)
close(cl[i]);
    close(sockfd);
}
