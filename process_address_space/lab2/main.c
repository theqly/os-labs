#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>


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

int main(int argc, char** argv){
    printf("watch -n 0.1 cat /proc/%d/maps\n", getpid());
    pid_t pid = getpid();
    //sleep(10);
    //stack_overflow(0);
    heap_overflow();

    //signal(SIGSEGV, );
    void* region = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    mprotect(region, 10 * sysconf(_SC_PAGE_SIZE), PROT_NONE);
    char try_to_read = *((char*)region);
    *((char*)region) = 1;
    void *unmap_start = (void*)((size_t)region + 4* sysconf(_SC_PAGE_SIZE));
    munmap(unmap_start, 3 * sysconf(_SC_PAGE_SIZE));
    return 0;
}

#endif
