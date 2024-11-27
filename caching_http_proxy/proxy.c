#include <arpa/inet.h>
#include <curl/curl.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define CACHE_SIZE_LIMIT 104857600

#define LOG_LEVEL 1

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

typedef struct CacheEntry {
  char *url;
  char *data;
  size_t size;
  int is_complete;
  pthread_mutex_t lock;
  struct CacheEntry *next;
} CacheEntry;

CacheEntry *cache_head = NULL;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t current_cache_size = 0;

typedef struct {
  char *memory;
  size_t size;
} MemoryBlock;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t total_size = size * nmemb;
  MemoryBlock *mem = (MemoryBlock *)userp;

  char *new_memory = realloc(mem->memory, mem->size + total_size + 1);
  if (new_memory == NULL) {
    fprintf(stderr, "Not enough memory!\n");
    return 0;
  }

  mem->memory = new_memory;
  memcpy(&(mem->memory[mem->size]), contents, total_size);
  mem->size += total_size;
  mem->memory[mem->size] = '\0';

  return total_size;
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
  pthread_mutex_lock(&cache_mutex);
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

      current_cache_size -= curr->size;
      free(curr->url);
      free(curr->data);
      pthread_mutex_destroy(&curr->lock);
      free(curr);
    }
  }
  pthread_mutex_unlock(&cache_mutex);
}

char *fetch_url(const char *url, size_t *data_size, int *http_status) {
  MemoryBlock chunk = {.memory = NULL, .size = 0};

  CURL *curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize libcurl\n");
    *http_status = 500;
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  const CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    *http_status = 502;
    free(chunk.memory);
    curl_easy_cleanup(curl);
    return NULL;
  }

  long response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  *http_status = (int)response_code;

  if (response_code >= 400) {
    fprintf(stderr, "Server returned error: %ld\n", response_code);
    free(chunk.memory);
    curl_easy_cleanup(curl);
    return NULL;
  }

  *data_size = chunk.size;
  curl_easy_cleanup(curl);
  return chunk.memory;
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

void *handle_client(void *args) {
  int sock = *(int *)args;
  free(args);

  char buffer[BUFFER_SIZE];
  while (1) {
    log_message("New cycle", 3);
    ssize_t bytes_read = read_full_request(sock, buffer, sizeof(buffer));
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

    pthread_mutex_lock(&cache_mutex);
    CacheEntry *entry = find_in_cache(url);
    if (entry && entry->is_complete) {
      log_message("url found in cache", 1);
      pthread_mutex_unlock(&cache_mutex);
      send(sock, entry->data, entry->size, 0);
      continue;
    }

    int http_status;
    if (!entry) {
      log_message("url not found in cache", 1);
      entry = malloc(sizeof(CacheEntry));
      entry->url = strdup(url);
      entry->data = NULL;
      entry->size = 0;
      entry->is_complete = 0;
      pthread_mutex_init(&entry->lock, NULL);
      entry->next = cache_head;
      cache_head = entry;
    }
    pthread_mutex_unlock(&cache_mutex);

    pthread_mutex_lock(&entry->lock);
    if (!entry->is_complete) {
      log_message("entry incomplete", 2);

      size_t data_size;
      entry->data = fetch_url(url, &data_size, &http_status);
      if (!entry->data) {
        send_error(sock, http_status);
        pthread_mutex_unlock(&entry->lock);
        break;
      }

      entry->size = data_size;
      entry->is_complete = 1;

      log_message("before lock mutex", 3);
      pthread_mutex_unlock(&entry->lock);

      //pthread_mutex_lock(&cache_mutex);
      current_cache_size += data_size;
      evict_cache_if_needed();
      //pthread_mutex_unlock(&cache_mutex);
    } else {
      pthread_mutex_unlock(&entry->lock);
    }

    log_message("before if", 3);

    if (entry->data) {
      char http_response[BUFFER_SIZE];
      snprintf(http_response, sizeof(http_response),
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: %zu\r\n"
               "Connection: close\r\n\r\n",
               entry->size);
      send(sock, http_response, strlen(http_response), 0);
      send(sock, entry->data, entry->size, 0);
    } else {
      send_error(sock, http_status);
      pthread_mutex_unlock(&entry->lock);
      break;
    }
    pthread_mutex_unlock(&entry->lock);
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
    pthread_create(&tid, NULL, handle_client, client_socket);
    pthread_detach(tid);
  }

  close(server_socket);
  return 0;
}
