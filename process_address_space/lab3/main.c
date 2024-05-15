#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_HEAP_SIZE 1024

struct info {
    size_t size;
    struct info* next;
    char is_available;
};

void* heap_start;

void prepare(){
    int fd = open("malloc.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open error");
        return;
    }

    if (ftruncate(fd, MAX_HEAP_SIZE) == -1) {
        perror("ftruncate error");
        close(fd);
        return;
    }

    void* heap = mmap(NULL, MAX_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(heap == MAP_FAILED){
        perror("mmap error");
        exit(1);
    }
    heap_start = heap;
    printf("info size: %lu\n", sizeof(struct info));
    ((struct info*)heap_start)->size = MAX_HEAP_SIZE - sizeof(struct info);
    ((struct info*)heap_start)->is_available = 1;
    ((struct info*)heap_start)->next = NULL;
}

void* my_malloc(size_t size){
    if(heap_start == NULL) prepare();
    void* memory = NULL;

    struct info* cur_info = (struct info*)heap_start;

    while(cur_info != NULL){
        printf("wanted size: %lu, is available = %d, size = %lu\n", size, cur_info->is_available, cur_info->size);
        if(cur_info->is_available && cur_info->size >= size){
            struct info* next = (struct info*)((char*)cur_info + sizeof(struct info) + size);
            if((char*)next >= (char*)heap_start + MAX_HEAP_SIZE) cur_info->next = NULL;
            else {
                next->size = cur_info->size - sizeof(struct info) - size;
                next->is_available = 1;
                cur_info->next = next;
            }

            cur_info->size = size;
            cur_info->is_available = 0;

            memory = (void*)(cur_info + 1);
            break;
        }
        cur_info = cur_info->next;
    }

    if(memory == NULL){
        fprintf(stderr, "malloc: cant find a memory\n");
        return memory;
    }

    printf("give %lu of memory\n", size);
    return memory;
}

void my_free(void* memory){
    if(memory == NULL) return;
    struct info* to_free = (struct info*)memory - 1;
    if(to_free->is_available == 1){
        fprintf(stderr, "free: its a freed memory\n");
    }

    to_free->is_available = 1;
    if(to_free->next != NULL && to_free->next->is_available){
        to_free->size += to_free->next->size + sizeof(struct info);
        to_free->next = to_free->next->next;
    }
}

int main(){
    char* arr1 = my_malloc(1024);
    if(arr1 == NULL) return 1;
    char* arr = my_malloc(1);
    if(arr == NULL) return 2;
    arr1[31] = 54;
    for(int i = 0; i < 1024; ++i){
        arr1[i] = i;
    }
    printf("arr[31] = %d\n", arr1[31]);
    my_free(arr);
    my_free(arr1);
    char* arr3 = my_malloc(4047);
    printf("arr[31] = %d\n", arr1[31]);
    if(arr3 == NULL) return 1;
    char* arr4 = my_malloc(1);
    if(arr4 == NULL) return 2;

    my_free(arr3);
    my_free(arr4);
    return 0;
}