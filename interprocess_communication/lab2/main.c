#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>

int main(){
    int value = 0;
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1){
        perror("pipe error");
        return 1;
    }

    pid_t reader = fork();
    if(reader < 0){
        perror("fork error");
        return 1;
    } else if (reader == 0){
        close(pipe_fd[1]);
        int last_value = 0;
        while (1) {
            read(pipe_fd[0], &value, sizeof(int));
            if(value != last_value + 1)
                printf("sequence error: expected %d, but got %d\n", last_value + 1, value);
            else printf("all right: expected %d and got %d\n", last_value + 1, value);
            last_value = value;
            sleep(1);
        }
    } else {
        close(pipe_fd[0]);
        while (1){
            value++;
            write(pipe_fd[1], &value, sizeof(int));
            if(value == INT_MAX) value = 0;
            sleep(1);
        }
    }

    return 0;
}