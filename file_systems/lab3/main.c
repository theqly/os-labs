#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char** argv){
    if (argc != 2){
        fprintf(stderr, "wrong number of args\n");
        return 1;
    }

    pid_t pid;
    char path[256];

    if (strcmp(argv[1], "self") != 0) {
        pid = atoi(argv[1]);
        snprintf(path, sizeof(path), "/proc/%d/pagemap", pid);
    } else {
        snprintf(path, sizeof(path), "/proc/self/pagemap");
        pid = getpid();
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("error with opening file");
        return 1;
    }

    //printf("reading pagemap file for PID %d: %s\n", pid, path);

    uint64_t entry;
    ssize_t read_bytes;
    uintptr_t virtual_address = 0;

    while ((read_bytes = read(fd, &entry, sizeof(entry))) > 0) {

    }

    if (read_bytes < 0) {
        fprintf(stderr, "Error reading %s: %s\n", path, strerror(errno));
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}