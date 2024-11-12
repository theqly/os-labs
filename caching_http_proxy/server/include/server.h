#ifndef SERVER_H
#define SERVER_H

#define PORT 12345
#define BUFFER_SIZE 4096

int parse_request(char *request, char **host, int *port);
int read_request(int client_socket, char **request);

int target_connection(char* host, int port);
void* handle_client(void* args);
void server_run();

#endif //SERVER_H
