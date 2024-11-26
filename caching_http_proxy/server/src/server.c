#include "server.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int target_connection(char *host, int port) {
  struct hostent *hp;
  struct sockaddr_in server_addr;
  int sock;

  if ((hp = gethostbyname(host)) == NULL) {
    herror("gethostbyname");
    return -1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr = *((struct in_addr *)hp->h_addr_list[0]);
  memset(&(server_addr.sin_zero), '\0', sizeof(server_addr.sin_zero));

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    close(sock);
    return -1;
  }

  return sock;
}

int read_request(int client_socket, char **request) {
  char buffer[BUFFER_SIZE];
  ssize_t request_len = 0;
  ssize_t n = 0;

  *request = NULL;

  while (1) {
    n = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0)
      return -1;

    *request = realloc(*request, request_len + n);
    if (*request == NULL) {
      perror("realloc");
      return -1;
    }

    memcpy(*request + request_len, buffer, n);
    request_len += n;

    if (strstr(*request, "\r\n\r\n") != NULL) {
      break;
    }
  }

  request[request_len] = '\0';

  return 0;
}

int parse_request(char *request, char **host, int *port) {
  char *url_start = strstr(request, "Host: ");
  if (url_start == NULL) {
    url_start = strstr(request, "host: ");
  }

  if (url_start == NULL)
    return -1;

  url_start += 6;
  char *url_end = strstr(url_start, "\r\n");
  if (url_end == NULL)
    return -1;

  size_t host_len = url_end - url_start;
  *host = malloc(host_len + 1);
  if (*host == NULL)
    return -1;

  strncpy(*host, url_start, host_len);
  (*host)[host_len] = '\0';

  char *colon_pos = strchr(*host, ':');
  if (colon_pos != NULL) {
    *colon_pos = '\0';
    *port = atoi(colon_pos + 1);
  } else {
    *port = 80;
  }

  return 0;
}

void *handle_client(void *args) {
  int client_socket = *((int *)args);
  char buffer[BUFFER_SIZE];
  char *request = NULL;
  char *host = NULL;
  int port = 80;

  if (read_request(client_socket, &request) < 0) {
    fprintf(stderr, "Error reading request\n");
    close(client_socket);
    return NULL;
  }

  printf("Request from client:\n%s\n", request);

  if (parse_request(request, &host, &port) < 0) {
    fprintf(stderr, "Invalid request\n");
    close(client_socket);
    free(request);
    return NULL;
  }

  int target_socket = target_connection(host, port);
  if (target_socket < 0) {
    fprintf(stderr, "Error connecting to server\n");
    close(client_socket);
    free(host);
    free(request);
    return NULL;
  }

  printf("Sending request to target server:\n%s\n", request);

  send(target_socket, request, strlen(request), 0);

  ssize_t n;
  while ((n = recv(target_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
    send(client_socket, buffer, n, 0);
    printf("Received request from target server:\n%s\n", buffer);
  }

  if(host != NULL) free(host);
  if(request != NULL) free(request);

  close(target_socket);
  close(client_socket);
  return NULL;
}

void server_run() {
  int server_fd, client_fd;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return;
  }

  const int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    perror("setsockopt");
    close(server_fd);
    return;
  }

  struct sockaddr_in serv_addr;
  const int serv_addr_len = sizeof(serv_addr);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&serv_addr, serv_addr_len) == -1) {
    perror("bind");
    close(server_fd);
    return;
  }

  if (listen(server_fd, MAX_USER_COUNT) == -1) {
    perror("listen");
    close(server_fd);
    return;
  }

  printf("Server started on port %d.\n", PORT);

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  while (1) {
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                            (socklen_t *)&client_addr_len)) < 0) {
      perror("accept");
      continue;
    }

    printf("new connection\n");

    pthread_t tid;
    if (pthread_create(&tid, NULL, &handle_client, (void *)&client_fd) != 0) {
      perror("pthread_create");
      continue;
    }
    pthread_detach(tid);
  }
}