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

int target_connection(const char *host, int port) {
  struct addrinfo hints, *res, *rp;
  char port_str[6];
  int sock;

  snprintf(port_str, sizeof(port_str), "%d", port);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port_str, &hints, &res) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == -1) continue;

    if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
    close(sock);
  }

  freeaddrinfo(res);

  if (rp == NULL) {
    fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
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
    if (n <= 0) {
      perror("recv");
      return -1;
    }

    char *temp = realloc(*request, request_len + n + 1);
    if (!temp) {
      perror("realloc");
      return -1;
    }
    *request = temp;

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
  const char *host_start = strstr(request, "Host: ");
  if (!host_start) host_start = strstr(request, "host: ");
  if (!host_start) return -1;


  host_start += 6;
  char *host_end = strstr(host_start, "\r\n");
  if (host_end == NULL)
    return -1;

  size_t host_len = host_end - host_start;
  *host = malloc(host_len + 1);
  if (*host == NULL)
    return -1;

  strncpy(*host, host_start, host_len);
  (*host)[host_len] = '\0';

  char *colon_pos = strchr(*host, ':');
  if (colon_pos != NULL) {
    *colon_pos = '\0';
    *port = atoi(colon_pos + 1);
    if (*port <= 0) *port = 80;
  } else {
    *port = 80;
  }

  return 0;
}

void *handle_client(void *args) {
  int client_socket = *((int *)args);

  while (1) {
    char *request = NULL;
    char *host = NULL;
    int port = 80;

    if (read_request(client_socket, &request) < 0) {
      fprintf(stderr, "Error reading request or connection closed\n");
      break;
    }

    if (parse_request(request, &host, &port) < 0) {
      fprintf(stderr, "Invalid request\n");
      free(request);
      break;
    }

    int target_socket = target_connection(host, port);
    if (target_socket < 0) {
      fprintf(stderr, "Error connecting to target server\n");
      free(request);
      free(host);
      break;
    }

    // Модификация заголовков
    char *connection_header = strstr(request, "\r\nConnection:");
    if (connection_header) {
      // Заменяем Connection на close
      snprintf(connection_header, 18, "\r\nConnection: close");
    }

    send(target_socket, request, strlen(request), 0);

    // Прокидываем данные от сервера клиенту
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while ((n = recv(target_socket, buffer, BUFFER_SIZE, 0)) > 0) {
      send(client_socket, buffer, n, 0);
    }

    free(request);
    free(host);
    close(target_socket);

    // Проверяем, нужно ли закрыть соединение
    if (strstr(request, "Connection: close") || strstr(request, "connection: close")) {
      break;
    }
  }

  close(client_socket);
  return NULL;
}


void server_run() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

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

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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
      close(client_fd);
      break;
    }
    pthread_detach(tid);
  }

  close(server_fd);

}