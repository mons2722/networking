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
#define max 10

int num=0;
struct clients
{ int fd;
  int no;
  char name[100];
}cl[max];
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
    if (len > 0 && client_key[len - 1] == '\n') 
    {
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

void send_frame (const uint8_t *frame, size_t length, int connfd) 
{
    ssize_t bytes_sent = send (connfd, frame, length, 0);
    if (bytes_sent == -1)
        perror("Send failed");
    else
        printf("Pong Frame sent to client\n");
}

void send_pong(const char *payload, size_t payload_length, int connfd)
{
    uint8_t pong_frame [128];
    pong_frame [0] = 0xA;
    pong_frame [1] = (uint8_t) payload_length;
    memcpy (pong_frame + 2, payload, payload_length);
    send_frame (pong_frame, payload_length + 2, connfd);
}

void handle_ping (const uint8_t *data, size_t length, int connfd)
{
    char ping_payload [126];
    memcpy (ping_payload, data + 2, length - 2);
    ping_payload [length - 2] = '\0';
    send_pong (ping_payload, length - 2, connfd);
}

// Function to decode the header of a WebSocket frame
int decode_websocket_frame_header(
    uint8_t *frame_buffer,
    uint8_t *fin,
    uint8_t *opcode,
    uint8_t *mask,
    uint64_t *payload_length
) 
{
    // Extract header bytes and mask
    *fin = (frame_buffer [0] >> 7) & 1;
    *opcode = frame_buffer [0] & 0x0F;
    *mask = (frame_buffer [1] >> 7) & 1;
    int n = 0;
    
    // Calculate payload length based on header type
    *payload_length = frame_buffer [1] & 0x7F;
    if (*payload_length == 126) 
    {
        n = 1;
        *payload_length = *(frame_buffer + 2);
        *payload_length <<= 8;
        *payload_length |= *(frame_buffer + 3);
    } 
    else if (*payload_length == 127) 
    {
        n = 2;
        *payload_length = 0;
        for (int i = 2; i < 10; ++i)
            *payload_length = (*payload_length << 8) | *(frame_buffer + i);
    }

    return  (2 + (n == 1 ? 2 : (n == 2 ? 8 : 0)));
}

int process_websocket_frame (uint8_t *data, size_t length, char **decoded_data, int connfd,int index)
{
    uint8_t fin, opcode, mask;
    uint64_t payload_length;
    uint8_t* masking_key;

    int header_size = decode_websocket_frame_header (data, &fin, &opcode, &mask, &payload_length);
    if (header_size == -1) 
    {
        printf ("Error decoding WebSocket frame header\n");
        return -1;
    }
    
    if (mask)
    {
        masking_key = header_size + data;
        header_size += 2;
    }
    header_size += 2;
    
    size_t payload_offset = header_size; 
    if (opcode == 0x9) 
    {
        handle_ping (data, length, connfd);
        *decoded_data = NULL;
        return 0;
    } 
    else if (opcode == 0x8) 
    {  
        printf("%s disconnected!!\n",cl[index].name);
	for(int i=index;i<num-1;i++)
	   cl[i]=cl[i+1];
	num--;
	if(num==0)
	   printf("No clients Connected!!\n");
	else
	{
        printf("\nConnected Clients:\n");
	for(int i=0;i<num;i++)
	 printf("%s\n",cl[i].name);
         }
        return -1;
    }

    *decoded_data = (char *)malloc (payload_length + 1);
    
    if (mask)
        for (size_t i = 0; i < payload_length; ++i)
         (*decoded_data) [i] = data [payload_offset + i] ^ masking_key [i % 4];

    (*decoded_data) [payload_length] = '\0';
    return 0;
}

void generate_random_mask (uint8_t *mask) 
{
    srand (time (NULL));

    // Generate a random 32-bit mask
    for (size_t i = 0; i < 4; ++i)
        mask [i] = rand () & 0xFF;
}

// Function to mask payload data
void mask_payload (uint8_t *payload, size_t payload_length, uint8_t *mask) 
{
    for (size_t i = 0; i < payload_length; ++i)
        payload [i] ^= mask [i % 4];
}

// Function to encode a complete WebSocket frame
int encode_frame (
    uint8_t fin,
    uint8_t opcode,
    uint8_t mask,
    uint64_t payload_length,
    uint8_t *payload,
    uint8_t *frame_buffer
)
{
    // Calculate header size based on payload length
    int header_size = 2;
    if (payload_length <= 125)
    {
        // Short form
    }
    else if (payload_length <= 65535)
    {
        // Medium form (2 additional bytes)
        header_size += 2;
    }
    else
    {
        // Long form (8 additional bytes)
        header_size += 8;
    }

    // Encode header bytes
    frame_buffer [0] = (fin << 7) | (opcode & 0x0F);
    frame_buffer [1] = mask << 7;
    if (payload_length <= 125)
        frame_buffer[1] |= payload_length;
    else if (payload_length <= 65535)
    {
        frame_buffer [1] |= 126;
        frame_buffer [2] = (payload_length >> 8) & 0xFF;
        frame_buffer [3] = payload_length & 0xFF;
    }
    else
    {
        frame_buffer [1] |= 127;
        uint64_t n = payload_length;
        for (int i = 8; i >= 1; --i)
        {
            frame_buffer [i + 1] = n & 0xFF;
            n >>= 8;
        }
    }

    // Mask payload if requested
    if (mask)
    {
        generate_random_mask (frame_buffer + header_size - 4);
        mask_payload (payload, payload_length, frame_buffer + header_size - 4);
    }

    // Copy payload after header
    memcpy (frame_buffer + header_size, payload, payload_length);
    return header_size + payload_length; // Total frame size
}

// Function to handle WebSocket data
void *handle_client(void *arg)
{ 
  int client=*((int *)arg);
  char buf[bufsize];
  char *decoded=NULL;
  int index;
  while(1)
  { 
    ssize_t bytes=recv(client,buf,sizeof(buf),0);
    buf[bytes]='\0';
//     printf("!!!%s\n",buf);
       
     for(int i=0;i<num;i++)
    { if(client==cl[i].fd)
         { index=i;
           break;
         }
    }

     int status = process_websocket_frame (buf, bytes,&decoded, client,index);
        if (status == -1)
           break;
        else if (status != 0) 
        {
            printf("Error processing WebSocket frame\n");
            close(client);
            continue;
        } 
    
    decoded[strlen(decoded)]='\0';
    char parts[3][100];
    int n=0,j;
    for(int i=0;i<strlen(decoded);i++)
    {
      j=0;
      while(decoded[i]!=':'&&decoded[i]!='\0')
      { parts[n][j++]=decoded[i++];
      }
      parts[n][j]='\0';
      n++;
    }
   	
    printf("Message Received from %s: %s\n",cl[index].name,decoded);
   
    char encoded[bufsize];
    int enc_size;
    
    char message[1024];
    if(num>1)
    {
    sprintf(message,"%s: %s",cl[index].name,decoded);
    
     //broadcast message
    enc_size=encode_frame(1,1,0,strlen(message),(uint8_t *)message,encoded);
    for(int i=0;i<num;i++)
    { if(cl[i].fd!=client)
	  send(cl[i].fd,encoded,enc_size,0);
            }
    printf("Message sent to all clients!!\n");    
    }
    else 
    { //revert sending error to client
      strcpy(message,"No Active Users!Message not sent.");
      enc_size=encode_frame(1,1,0,strlen(message),(uint8_t *)message,encoded);
      send(client,encoded,enc_size,0);   
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

// extraxt user name from the JSON request

void extract_username(int client,int index)
{
  char data[bufsize];
  int bytes=recv(client,data,bufsize,0);
  data[bytes]='\0';
  char *username=NULL;
  int status=process_websocket_frame (data,bytes, &username,client,index);
  if(status==0)
  strcpy(cl[index].name,username);
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
	cl[num].fd=client;
	cl[num].no=num+1;
	extract_username(cl[num].fd,num);
	printf("Connected to %s\n",cl[num].name);
	num++;
	
	pthread_t thread_id;
	 if (pthread_create(&thread_id, NULL, handle_client, &client) != 0) 
	 {
            perror("Thread creation failed");
            close(client);
            continue;
        }
	 pthread_detach(thread_id);
}
    close(sockfd);
}
