#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>


#if 0
int main(int argc, char** argv){
    printf("pid: %d\n", getpid());
    sleep(1);
    execv(argv[0], argv);
    printf("Hello, world!\n");
    return 0;
}

#else

void stack_overflow(int i){
    if(i == 50) return;
    printf("stack overflow\n");
    char very_big_array_on_stack[1024*512];
    sleep(1);
    stack_overflow(i + 1);
}

void heap_overflow(){
    printf("heap overflow\n");
    void* pointers[10];
    for(int i = 0; i < 10; ++i){
        pointers[i] = malloc(1024* 512);
        sleep(1);
    }
    for(int i = 0; i < 10; ++i){
        free(pointers[i]);
    }
}

void signal_handler(int signal_num){
    printf("Segmentation fault occurred");
    exit(1);
}

int main(int argc, char** argv){
    printf("watch -n 0.1 cat /proc/%d/maps\n", getpid());
    pid_t pid = getpid();
    //sleep(10);
    //stack_overflow(0);
    //heap_overflow();

    signal(SIGSEGV, signal_handler);
    void* region = mmap(NULL,10 * sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(region == MAP_FAILED){
        perror("mmap error");
        return 1;
    }

    /*if(mprotect(region, 10 * sysconf(_SC_PAGE_SIZE), PROT_NONE)){
        perror("mprotect error");
        return 1;
    }*/

    char try_to_read = *((char*)region);
    if(try_to_read != -1) printf("region read\n");

    char* try_to_write = "try to write";
    strcpy((char*) region, try_to_write);
    printf("region: %s\n", (char*) region);

    if(munmap(region + 4 * sysconf(_SC_PAGE_SIZE), 3 * sysconf(_SC_PAGE_SIZE))){
        perror("munmap error");
        return 1;
    }
    return 0;
}

#endif
