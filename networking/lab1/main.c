#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static void tcp_server() {
    // create socket
    const int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        perror("socket error:");
        return;
    }

    // bind to open port
    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(fd, (struct sockaddr*) &s_addr, sizeof(s_addr)) == -1){
        perror("bind error:");
        return;
    }

    // read port
    socklen_t addr_len = sizeof(s_addr);
    getsockname(fd, (struct sockaddr*) &s_addr, &addr_len);
    printf("server is on port %d\n", (int) ntohs(s_addr.sin_port));

    if (listen(fd, 2) != 0) {
        perror("listen error:");
        return;
    }

    // accept incoming connection

    struct sockaddr_storage c_addr;
    socklen_t c_addr_len = sizeof(c_addr);
    int c_fd, len;

    // read from client with recv!
    char buf[1024];
    while (1) {
        c_fd = accept(fd, (struct sockaddr*) &c_addr, &c_addr_len);
        while((len = recv(c_fd, buf, sizeof(buf), 0)) != 0) {
            printf("client says:\n    %s\n", buf);
            send(c_fd, buf, len, 0);
        }
        //send(cfd, buf, sizeof(buf), 0);
    }

    // print without looking


    close(c_fd);
    close(fd);
}

static void client(int port) {
    const int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short) port);

    // connect to local machine at specified port
    char addrstr[NI_MAXHOST + NI_MAXSERV + 1];
    snprintf(addrstr, sizeof(addrstr), "127.0.0.1:%d", port);

    // parse into address
    inet_pton(AF_INET, addrstr, &addr.sin_addr);

    // connect to server
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("connect error:");
        return;
    }

    // say hey with send!
    const char *msg = "the client says hello!";
    send(fd, msg, strlen(msg) + 1, 0);

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && !strcmp(argv[1], "client")) {
        if (argc != 3) {
            fprintf(stderr, "not enough args!");
            return -1;
        }

        int port;
        sscanf(argv[2], "%d", &port);

        client(port);
    } else {
        tcp_server();
    }

    return 0;
}