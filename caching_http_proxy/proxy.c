#include <arpa/inet.h>
#include <curl/curl.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define CACHE_SIZE_LIMIT 104857600

#define LOG_LEVEL 1

typedef struct CacheEntry {
  char *url;
  int http_status;
  char *data;
  char *data_type;
  size_t full_size;
  size_t cur_size;
  char is_complete;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  struct CacheEntry *next;
} CacheEntry;

CacheEntry *cache_head = NULL;
pthread_rwlock_t cache_lock = PTHREAD_RWLOCK_INITIALIZER;
size_t current_cache_size = 0;

void sigpipe_handler(int sig) {
  printf("got sigpipe\n");
}

CacheEntry *find_in_cache(const char *url) {
  CacheEntry *curr = cache_head;
  while (curr) {
    if (strcmp(curr->url, url) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

void evict_cache_if_needed() {
  while (current_cache_size > CACHE_SIZE_LIMIT) {
    CacheEntry *prev = NULL, *curr = cache_head;
    while (curr && curr->next) {
      prev = curr;
      curr = curr->next;
    }
    if (curr) {
      if (prev) {
        prev->next = NULL;
      } else {
        cache_head = NULL;
      }

      pthread_mutex_lock(&curr->mutex);

      current_cache_size -= curr->full_size;
      pthread_mutex_destroy(&curr->mutex);
      pthread_cond_destroy(&curr->cond);
      pthread_mutex_destroy(&curr->mutex);
      free(curr->url);
      free(curr->data);
      free(curr);
    }
  }
}

void log_message(const char *message, const int log_level) {
  if (log_level <= LOG_LEVEL) {
    printf("*** LOG: %s ***\n", message);
  }
}

void send_error(const int sock, const int http_status) {
  char error_response[256];
  snprintf(error_response, sizeof(error_response),
           "HTTP/1.1 %d Error\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %d\r\n"
           "Connection: close\r\n\r\n"
           "Error fetching URL.\n",
           http_status, (int)strlen("Error fetching URL.\n"));
  send(sock, error_response, strlen(error_response), 0);
}

ssize_t send_data(const int sock, const char *data, const char *data_type,
              const unsigned long size) {
  char http_response[BUFFER_SIZE];
  snprintf(http_response, sizeof(http_response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %zu\r\n"
           "Connection: close\r\n\r\n",
           data_type, size);
  if (send(sock, http_response, strlen(http_response), 0) < 0) return -1;
  return send(sock, data, size, 0);
}

ssize_t read_full_request(int sock, char *buffer, size_t buffer_size) {
  ssize_t total_read = 0;
  while (total_read < buffer_size - 1) {
    ssize_t bytes_read =
        recv(sock, buffer + total_read, buffer_size - total_read - 1, 0);
    if (bytes_read <= 0)
      return bytes_read;

    total_read += bytes_read;
    buffer[total_read] = '\0';

    if (strstr(buffer, "\r\n\r\n"))
      break;
  }
  return total_read;
}

size_t header_callback(char *buffer, size_t size, size_t nitems,
                       void *userdata) {
  CacheEntry *entry = (CacheEntry *)userdata;
  size_t total_size = size * nitems;

  pthread_mutex_lock(&entry->mutex);

  if (entry->http_status == 0 &&
      sscanf(buffer, "HTTP/1.1 %d", &entry->http_status) == 1) {
    pthread_mutex_unlock(&entry->mutex);
    return total_size;
  }

  if (strstr(buffer, "Content-Type:") == buffer) {
    char *content_type = strchr(buffer, ':') + 1;
    while (*content_type == ' ')
      content_type++;
    entry->data_type = strndup(content_type, strcspn(content_type, "\r\n"));
  }
  if (strstr(buffer, "Content-Length:") == buffer) {
    sscanf(buffer, "Content-Length: %ld", &entry->full_size);
  }

  pthread_mutex_unlock(&entry->mutex);

  return total_size;
}

size_t body_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  CacheEntry *entry = (CacheEntry *)userdata;
  size_t total_size = size * nitems;

  pthread_mutex_lock(&entry->mutex);

  entry->data = realloc(entry->data, entry->cur_size + total_size + 1);

  if (entry->data == NULL) {
    fprintf(stderr, "Not enough memory!\n");
    pthread_mutex_unlock(&entry->mutex);
    return 0;
  }

  memcpy(entry->data + entry->cur_size, buffer, total_size);
  entry->cur_size += total_size;
  entry->data[entry->cur_size] = '\0';

  pthread_mutex_unlock(&entry->mutex);

  pthread_cond_broadcast(&entry->cond);

  return total_size;
}

int fetch_header(const char *url, CacheEntry *entry) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize libcurl\n");
    pthread_mutex_lock(&entry->mutex);
    entry->http_status = 502;
    pthread_mutex_unlock(&entry->mutex);
    return -1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, entry);

  const CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    pthread_mutex_lock(&entry->mutex);
    entry->http_status = 502;
    pthread_mutex_unlock(&entry->mutex);
    curl_easy_cleanup(curl);
    return -1;
  }

  curl_easy_cleanup(curl);
  return 0;
}

int fetch_data(const char *url, CacheEntry *entry) {

  CURL *curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize libcurl\n");
    return -1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, entry);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  const CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    pthread_mutex_lock(&entry->mutex);
    entry->http_status = 502;
    pthread_mutex_unlock(&entry->mutex);
    curl_easy_cleanup(curl);
    return -1;
  }

  pthread_cond_broadcast(&entry->cond);

  pthread_mutex_lock(&entry->mutex);
  if(entry->full_size == 0) entry->full_size = entry->cur_size;
  entry->is_complete = 1;
  pthread_mutex_unlock(&entry->mutex);

  curl_easy_cleanup(curl);
  return 0;
}

int load_to_cache(const char *url, const int sock, CacheEntry *entry) {

  pthread_mutex_lock(&entry->mutex);
  size_t data_size = entry->full_size;
  pthread_mutex_unlock(&entry->mutex);

  if(fetch_data(url, entry) == -1) {
    pthread_mutex_lock(&entry->mutex);
    send_error(sock, entry->http_status);
    pthread_mutex_unlock(&entry->mutex);
    return -1;
  }

  pthread_rwlock_wrlock(&cache_lock);
  current_cache_size += data_size;
  evict_cache_if_needed();
  pthread_rwlock_unlock(&cache_lock);

  pthread_cond_broadcast(&entry->cond);

  return 0;
}

typedef struct {
  int sock;
  CacheEntry *entry;
} send_info;

void* send_data_from_cache(void* arg) {
  send_info info = *(send_info*)arg;
  free(arg);

  log_message("in send from cache", 1);

  char http_response[BUFFER_SIZE];
  pthread_mutex_lock(&info.entry->mutex);
  snprintf(http_response, sizeof(http_response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Connection: close\r\n\r\n",
           info.entry->data_type);
  pthread_mutex_unlock(&info.entry->mutex);

  log_message("before send header send from cache", 1);

  if (send(info.sock, http_response, strlen(http_response), 0) < 0) return NULL;

  size_t sent_bytes = 0;

  log_message("before while in send from cache", 1);

  pthread_mutex_lock(&info.entry->mutex);
  while (sent_bytes < info.entry->full_size) {
    if(info.entry->http_status >= 500) {
      break;
    }
    while(info.entry->cur_size == sent_bytes) {
      pthread_cond_wait(&info.entry->cond, &info.entry->mutex);
    }

    size_t cur_sent =
        send(info.sock, info.entry->data + sent_bytes, info.entry->cur_size - sent_bytes, 0);

    if(cur_sent < 0) return NULL;
    sent_bytes += cur_sent;
  }
  pthread_mutex_unlock(&info.entry->mutex);
  log_message("after while in send from cache", 1);
  return NULL;
}

void *handle_client(void *args) {
  const int sock = *(int *)args;
  free(args);
  signal(SIGPIPE, sigpipe_handler);

  while (1) {
    char buffer[BUFFER_SIZE];
    const ssize_t bytes_read = read_full_request(sock, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
      send_error(sock, 400);
      break;
    }

    char method[8], url[256], version[16];
    if (sscanf(buffer, "%s %s %s", method, url, version) != 3 ||
        strcmp(method, "GET") != 0) {
      send_error(sock, 400);
      break;
    }

    pthread_rwlock_rdlock(&cache_lock);
    CacheEntry *entry = find_in_cache(url);

    if (entry) {
      pthread_rwlock_unlock(&cache_lock);
      pthread_mutex_lock(&entry->mutex);

      if (entry->is_complete) {
        log_message("url found in cache", 1);
        send_data(sock, entry->data, entry->data_type, entry->full_size);
        pthread_mutex_unlock(&entry->mutex);
        break;
      }
      pthread_mutex_unlock(&entry->mutex);

      log_message("entry incomplete", 1);
      send_info* info = malloc(sizeof(send_info));
      info->sock = sock; info->entry = entry;
      send_data_from_cache(info);
      break;
    }
    pthread_rwlock_unlock(&cache_lock);

    pthread_rwlock_wrlock(&cache_lock);

    log_message("url not found in cache", 1);
    entry = malloc(sizeof(CacheEntry));
    entry->url = strdup(url);
    entry->data = NULL;
    entry->data_type = NULL;
    entry->full_size = 0;
    entry->cur_size = 0;
    pthread_mutex_init(&entry->mutex, NULL);
    pthread_cond_init(&entry->cond, NULL);
    entry->next = cache_head;
    cache_head = entry;

    pthread_rwlock_unlock(&cache_lock);

    if(fetch_header(url, entry) == -1) {
      send_error(sock, entry->http_status);
      break;
    }

    pthread_t tid;
    send_info* info = malloc(sizeof(send_info));
    info->sock = sock; info->entry = entry;
    pthread_create(&tid, NULL, send_data_from_cache, info);

    if(load_to_cache(url, sock, entry) == -1) break;
    pthread_join(tid, NULL);
    break;
  }
  close(sock);
  return NULL;
}

int main() {
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed");
    return 1;
  }

  const int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    perror("setsockopt");
    close(server_socket);
    return -1;
  }

  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr.s_addr = INADDR_ANY,
  };

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
    perror("Bind failed");
    close(server_socket);
    return 1;
  }

  if (listen(server_socket, 10) == -1) {
    perror("Listen failed");
    close(server_socket);
    return 1;
  }

  printf("Proxy server listening on port %d\n", PORT);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int *client_socket = malloc(sizeof(int));
    *client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (*client_socket == -1) {
      perror("Accept failed");
      free(client_socket);
      continue;
    }

    log_message("New client connected", 0);

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, client_socket)) {
      fprintf(stderr, "main: pthread_create() failed\n");
      send_error(*client_socket, 500);
      break;
    }
    pthread_detach(tid);
  }

  close(server_socket);
  return 0;
}
