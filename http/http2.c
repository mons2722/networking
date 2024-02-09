#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>

#define PORT 8080
#define MAX_CLIENTS 2

struct lws *clients[MAX_CLIENTS];

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

static struct lws_protocols protocols[] = {
    {
        "http-only",
        callback_http,
        0,
    },
    {
        "websockets",
        callback_websockets,
        0,
    },
    { NULL, NULL, 0, 0 }
};

void forward(struct lws *to, char *mes, size_t len) {
    printf("\n@@\n");
    lws_write(to, (unsigned char *)mes, len, LWS_WRITE_TEXT);
}

int main() {
    struct lws_context_creation_info info;
    struct lws_context *context;
    int port = PORT;
    const char *iface = NULL;
    int opts = 0;

    memset(&info, 0, sizeof info);
    info.port = port;
    info.iface = iface;
    info.protocols = protocols;
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.extensions = NULL;
    info.gid = -1;
    info.uid = -1;
    info.options = opts;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create libwebsockets context\n");
        return 1;
    }

    printf("Server started. Waiting for connections on port %d...\n", PORT);

    while (1) {
        lws_service(context, 50);
    }

    lws_context_destroy(context);

    return 0;
}

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            char *requested_uri = (char *)in;
            if (strncmp(requested_uri, "/get", 4) == 0) {
                // Handle GET request
                char *response = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/html\r\n\r\n"
                                 "<h1>Hello, this is a GET request!</h1>";
                lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);
            } else if (strncmp(requested_uri, "/post", 5) == 0) {
                // Handle POST request
                char *response = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: text/html\r\n\r\n"
                                 "<h1>Hello, this is a POST request!</h1>";
                lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);
            } else {
                // Error handling for unknown requests
                char *response = "HTTP/1.1 404 Not Found\r\n"
                                 "Content-Type: text/html\r\n\r\n"
                                 "<h1>404 Not Found</h1>"
                                 "<p>The requested URL was not found on this server.</p>";
                lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("WebSocket client connected\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i]) {
                    clients[i] = wsi;
                    break;
                }
            }
            break;
        case LWS_CALLBACK_RECEIVE:
            printf("Received message from client\n");
            // Forward the received message to the other client
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] != wsi && clients[i] != NULL) {
                    forward(clients[i], (char *)in, len);
                    break; // Forward to only one client
                }
            }
            break;
        case LWS_CALLBACK_CLOSED:
            printf("WebSocket client disconnected\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == wsi) {
                    clients[i] = NULL;
                    break;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}
