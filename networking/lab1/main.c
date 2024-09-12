#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static void udp_server(int port) {
    const int server_fd = socket(PF_INET, SOCK_DGRAM, 0);
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

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int len;

    char message[1024];
    while (1) {
        len = recvfrom(server_fd, message, sizeof(message), 0, (struct sockaddr*)&client_addr, &client_addr_len);
        printf("client says: %s", message);
        sendto(server_fd, message, len, 0, (struct sockaddr*)&client_addr, client_addr_len);
    }
}

static void client(int port) {
    const int server_fd = socket(PF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    char addrstr[NI_MAXHOST + NI_MAXSERV + 1];
    snprintf(addrstr, sizeof(addrstr), "127.0.0.1:%d", port);

    inet_pton(AF_INET, addrstr, &server_addr.sin_addr);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    char message[1024];
    while (1) {
        fputs("enter your message: ", stdout);
        fgets(message,1024, stdin);
        if(!strcmp(message, "quit\n")) break;
        sendto(server_fd, message, 1024, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        int len = recvfrom(server_fd, message, 1024, 0, (struct sockaddr*)&addr, &addr_len);
        message[len-1] = '\0';
        printf("message from server: %s", message);
    }

}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        fprintf(stderr, "Usage: ./lab1 <server or client> <port>");
        return 1;
    }
    printf("pid: %d\n", getpid());
    int port;
    sscanf(argv[2], "%d", &port);
    if(!strcmp(argv[1], "client"))  client(port);
    else udp_server(port);

    return 0;
}