#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>

#define PORT 8080
#define MAX_CLIENTS 2

struct lws *clients[MAX_CLIENTS];

void forward(struct lws *to, char *mes, size_t len) {
    printf("\n@@\n");
    lws_write(to, (unsigned char *)mes, len, LWS_WRITE_TEXT);
}

void websocket_handshake(struct lws *wsi, char *request, size_t len) {
    char response[] = "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "\r\n";
    lws_write(wsi, (unsigned char *)response, strlen(response), LWS_WRITE_HTTP);
}

void postreq(struct lws *wsi, char *p, size_t len) {
    printf("\nReceived Message:\n");
    int n = len, count = 1;
    printf("%d %d %d\n", (int)wsi, (int)clients[0], (int)clients[1]);
    if (wsi == clients[0])
        forward(clients[1], p, len);
    else
        forward(clients[0], p, len);
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

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_HTTP:
            handle_http_req(wsi, (char *)in, len);
            break;
        default:
            break;
    }
    return 0;
}

static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason,
                               void *user, void *in, size_t len)
{
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
            websocket_data(wsi, (char *)in, len);
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

static struct lws_protocols protocols[] = {
    {"http-only", callback_http, 0, 0},
    {"websocket", callback_websockets, 0, 0},
    {NULL, NULL, 0, 0}
};

int main() {
    struct lws_context_creation_info info;
    struct lws_context *context;

    memset(&info, 0, sizeof(info));
    info.port = PORT;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        printf("Failed to create libwebsockets context\n");
        return -1;
    }

    printf("Server started. Waiting for connections on port %d...\n", PORT);

    while (1) {
        lws_service(context, 50);
    }

    lws_context_destroy(context);

    return 0;
}

