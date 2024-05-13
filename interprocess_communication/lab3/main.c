#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


static void server(char* socket_file) {
    const int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_fd == -1) {
        perror("socket error");
        return;
    }


    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_file, sizeof(server_addr.sun_path) - 1);
    unlink(socket_file);

    if(bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        perror("bind error");
        close(server_fd);
        return;
    }

    printf("server is in file %s\n", socket_file);

    if (listen(server_fd, 3) != 0) {
        perror("listen error");
        close(server_fd);
        return;
    }

    struct sockaddr_un client_addr;
    int client_fd;
    ssize_t read_bytes;
    pid_t pid;

    char message[1024];
    while (1) {
        socklen_t client_addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if(client_fd == -1) {
            perror("accept error");
            close(client_fd);
            break;
        }
        printf("new connection with socket %d in file %s\n", client_fd, socket_file);
        pid = fork();
        if(pid == 0){
            client_addr_len = sizeof(client_addr);

            if(getpeername(client_fd, (struct sockaddr*) &client_addr, &client_addr_len) == -1){
                perror("getpeername error");
                close(client_fd);
                break;
            }

            while(1){
                read_bytes = recv(client_fd, message, 1024, 0);
                if(read_bytes <=0 ){
                    break;
                }
                if(send(client_fd, message, read_bytes, 0) == -1){
                    break;
                }
            }
        }

        if(pid < 0){
            perror("fork error");
            close(client_fd);
            continue;
        }

        close(client_fd);
    }

    close(server_fd);
    unlink(socket_file);
    return;
}

static void client(char* socket_file) {
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, socket_file, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("connect error");
        close(fd);
        return;
    }
    char message[1024];
    while (1) {
        fputs("enter your message: ", stdout);
        fgets(message,1024, stdin);
        if(!strcmp(message, "quit\n")) break;
        if(send(fd, message, 1024, 0) == -1) {
            perror("send error");
            break;
        }

        ssize_t read_bytes = recv(fd, message, 1024, 0);
        if(read_bytes <= 0) {
            if(read_bytes == 0) printf("server is not available\n");
            else perror("recv error");
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
        fprintf(stderr, "Usage: ./lab1 <server or client> <socket_file>");
        return 1;
    }
    if(!strcmp(argv[1], "client"))  client(argv[2]);
    else server(argv[2]);

    return 0;
}