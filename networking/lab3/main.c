#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

static void tcp_server(int port) {
    const int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd <= 0) {
        perror("socket error: ");
        return;
    }


    struct sockaddr_in server_addr;
    int server_addr_len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*) &server_addr, server_addr_len) == -1){
        perror("bind error: ");
        return;
    }

    printf("server is on port %d\n", (int) ntohs(server_addr.sin_port));

    if (listen(server_fd, 10) < 0) {
        perror("listen error: ");
        return;
    }

    int client_sockets[10], activity;
    struct pollfd fds[11];

    memset(client_sockets, 0, sizeof(client_sockets));
    memset(fds, 0, sizeof(fds));

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    char message[1024] = {0};
    int new_client_fd;
    while (1) {
        activity = poll(fds, 11, -1);
        if((activity < 0) && (errno != EINTR)) {
            perror("poll error: ");
            return;
        }
        if(fds[0].revents & POLLIN) {
            fds[0].revents = 0;
            if((new_client_fd = accept(server_fd, (struct sockaddr *)&server_addr, (socklen_t *)&server_addr_len)) < 0) {
                perror("accept error: ");
                return;
            }

            printf("new connection with socket %d on %s:%d\n", new_client_fd, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

            for(int i = 1; i < 11; ++i) {
                if(client_sockets[i-1] == 0) {
                    client_sockets[i-1] = new_client_fd;
                    fds[i].fd = new_client_fd;
                    fds[i].revents = POLLIN;
                    break;
                }
            }
        }

        for(int i = 1; i < 11; ++i) {
            if(client_sockets[i-1] > 0 && (fds[i].revents & POLLIN)) {
                fds[i].revents = 0;
                int read_bytes;
                if((read_bytes = recv(client_sockets[i-1], message, 1024, 0)) <= 0) {
                    if(read_bytes == 0) printf("host %d disconnected\n", client_sockets[i-1]);
                    else perror("recv error: ");
                    close(client_sockets[i-1]);
                    client_sockets[i-1] = 0;
                } else {
                    message[read_bytes] = '\0';
                    printf("client %d says: %s", client_sockets[i-1], message);
                    send(client_sockets[i-1], message, read_bytes, 0);
                    fds[i].events |= POLLIN;
                }
            }
        }

    }
}

void client(int port) {
    const int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    char addrstr[NI_MAXHOST + NI_MAXSERV + 1];
    snprintf(addrstr, sizeof(addrstr), "127.0.0.1:%d", port);

    inet_pton(AF_INET, addrstr, &addr.sin_addr);

    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("connect error: ");
        return;
    }
    char message[1024];
    while (1) {
        fputs("enter your message: ", stdout);
        fgets(message,1024, stdin);
        if(!strcmp(message, "quit\n")) break;
        if(send(fd, message, 1024, 0) <= 0) {
            perror("error with sending");
            return;
        }
        int read_bytes = recv(fd, message, 1024, 0);
        if(read_bytes <= 0) {
            if(read_bytes == 0) printf("server is not available\n");
            else perror("recv error: ");
            close(fd);
            break;
        }
        message[read_bytes] = '\0';
        printf("message from server: %s", message);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: ./lab1 <server or client> <port>");
        return 1;
    }
    int port;
    sscanf(argv[2], "%d", &port);
    if(!strcmp(argv[1], "client"))  client(port);
    else tcp_server(port);

    return 0;
}