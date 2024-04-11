#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int global = 1;

int main(){
    int local = 2;
    printf("global: %p -> %d\n", &global, global);
    printf("local: %p -> %d\n", &local, local);
    printf("pid: %d\n", getpid());

    pid_t pid = fork();

    if(pid == 0) {
        printf("child pid: %d\n", getpid());
        printf("parent pid: %d\n", getppid());
        printf("global in child process: %p -> %d\n", &global, global);
        printf("local in child process: %p -> %d\n", &local, local);
        global = 3;
        local = 4;
        printf("global in child process after change: %p -> %d\n", &global, global);
        printf("local in child process after change: %p -> %d\n", &local, local);

        sleep(10);
        exit(5);

    } else if(pid > 0){
        printf("global in parent process: %p -> %d\n", &global, global);
        printf("local in parent process: %p -> %d\n", &local, local);
        sleep(30);
        exit(0);
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)) {
            printf("child process exit code: %d\n", WEXITSTATUS(status));
        } else printf("child process has exited incorrect\n");

    } else {
        perror("fork error: ");
        return 1;
    }

    return 0;
}