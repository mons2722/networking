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

int cl1, cl2;

void forward(int to, char *mes) {
    printf("\n@@\n");
    send(to, mes, bufsize, 0);
}  

// Function to handle WebSocket handshake
void websocket_handshake(int client, char *request) {
    char response[bufsize];
    sprintf(response, 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "\r\n"
    );
    send(client, response, strlen(response), 0);
}

// Function to handle WebSocket data
void websocket_data(int client, char *data) {
    printf("WebSocket Data Received: %s\n", data);
    // Add your WebSocket data handling logic here
}

// Function to handle HTTP POST request
void postreq(int client, char *p) {
    printf("\nReceived Message:\n");
    int n = strlen(p), count = 1;
    printf("%d %d %d\n", client, cl1, cl2);
    if (client == cl1)
        forward(cl2, p);
    else
        forward(cl1, p);
    while ((count++) <= n) {
        if (*p == '+') {
            putchar(' ');
            p++;
            continue;
        }
        if (*p == '%') {
            putchar('\n');
            p += 6;
            count += 5; 
            continue;
        }
        putchar(*p++);
    }
    printf("\n");
}

void handle_http_req(int client, char buf[]) {
    printf("\n%s", buf);
    if (strstr(buf, "GET") != NULL) {
        // Handle WebSocket handshake
        websocket_handshake(client, buf);
    } else if (strstr(buf, "POST") != NULL) {
        // Find start of post_data
        char *start = strstr(buf, "\r\n\r\n");
        start += strlen("\r\n\r\n");
        // Move to start of actual data
        postreq(client, start);
    } else {
        // Handle other requests and send error message
        char *error = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        send(client, error, bufsize, 0);
    }
}

void *get_in_addr(struct sockaddr *s) {
    if (s->sa_family == AF_INET)
        return &(((struct sockaddr_in*)s)->sin_addr);
    else  
        return &(((struct sockaddr_in6*)s)->sin6_addr);
}

void main() {
    int sockfd;
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
    
    // Connect client 1
    sin_size = sizeof(caddr);
    cl1 = accept(sockfd, (struct sockaddr *)&caddr, &sin_size);
    if (cl1 == -1) {
        perror("Server:Accept\n");
        exit(1);
    } else {
        printf("\nConnected to Client 1");
    }

    // Connect client 2 
    cl2 = accept(sockfd, (struct sockaddr *)&caddr, &sin_size);
    if (cl2 == -1) {
        perror("Server:Accept\n");
        exit(1);
    } else {
        printf("\nConnected to Client 2");
    }
     
    while (1) {  
        recv(cl1, buf, bufsize, 0);
        handle_http_req(cl1, buf);
   
        recv(cl2, buf, bufsize, 0);
        handle_http_req(cl2, buf);
    }
  
    close(cl1);
    close(cl2);
    close(sockfd);
}
