#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>

int main(){
    int* region = mmap(NULL, sysconf(_SC_PAGE_SIZE),
                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(region == MAP_FAILED){
        perror("map error");
        return 1;
    }

    *region = 0;

    pid_t reader = fork();
    if(reader < 0){
        perror("fork error");
        return 1;
    } else if (reader == 0){
        int last_value = 0;
        while (1) {
            int cur_value = *region;
            if(cur_value != last_value + 1)
                printf("sequence error: expected %d, but got %d\n", last_value + 1, cur_value);
            else printf("all right: expected %d and got %d\n", last_value + 1, cur_value);
            last_value = cur_value;
            //sleep(1);
        }
    } else {
        while (1){
            (*region)++;
            if(*region == INT_MAX) *region = 0;
            //sleep(1);
        }
    }

    return 0;
}