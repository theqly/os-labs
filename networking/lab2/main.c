#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static void tcp_server(int port) {
    const int server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {
        perror("socket error: ");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        perror("bind error: ");
        return;
    }

    printf("server is on port %d\n", (int) ntohs(server_addr.sin_port));

    if (listen(server_fd, 2) != 0) {
        perror("listen error: ");
        return;
    }

    struct sockaddr_storage client_addr;
    socklen_t c_addr_len = sizeof(client_addr);
    int client_fd, len;
    pid_t pid;

    char message[1024];
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &c_addr_len);
        if(client_fd == -1) perror("accept error: ");
        pid = fork();
        if(pid == 0) {
            close(server_fd);
            while((len = recv(client_fd, message, sizeof(message), 0)) != 0) {
                printf("client says: %s", message);
                send(client_fd, message, len, 0);
            }
            close(client_fd);
            return;
        }

        close(client_fd);
    }
}

static void client(int port) {
    const int client_fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    char addrstr[NI_MAXHOST + NI_MAXSERV + 1];
    snprintf(addrstr, sizeof(addrstr), "127.0.0.1:%d", port);

    inet_pton(AF_INET, addrstr, &addr.sin_addr);

    if (connect(client_fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("connect error: ");
        return;
    }
    char message[1024];
    while (1) {
        fputs("enter your message: ", stdout);
        fgets(message,1024, stdin);
        if(!strcmp(message, "quit\n")) break;
        send(client_fd, message, 1024, 0);
        int len = recv(client_fd, message, 1024, 0);
        message[len] = '\0';
        printf("message from server: %s", message);
    }

    close(client_fd);
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