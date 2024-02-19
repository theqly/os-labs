#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "enter a programm and arguments");
        return 1;
    }
    pid_t child = fork();

    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], argv + 1);
        fprintf(stderr, "code after execpv (bad)");
        return 1;
    } else {
        int status;
        struct user_regs_struct regs;
        wait(&status);
        while (WIFSTOPPED(status)) {
            ptrace(PTRACE_GETREGS, child, NULL, &regs);

            long syscall = regs.orig_rax;
            printf("syscall %ld\n", syscall);

            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
            wait(&status);
        }
    }

    return 0;
}