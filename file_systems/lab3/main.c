#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define PAGEMAP_ENTRY 8
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

void read_pagemap(char * path_buf, unsigned long virt_addr){
    FILE* f = fopen(path_buf, "rb");
    if(!f){
        perror("fopen error");
        return;
    }

    unsigned long file_offset = virt_addr / sysconf(_SC_PAGESIZE) * PAGEMAP_ENTRY;

    if(fseek(f, file_offset, SEEK_SET)){
        perror("fseek error");
        return;
    }

    uint64_t physical_address = 0;
    unsigned char char_buf[PAGEMAP_ENTRY];
    int c;

    for(int i = 0; i < PAGEMAP_ENTRY; ++i){
        c = getc(f);
        char_buf[PAGEMAP_ENTRY - i - 1] = c;
    }

    for(int i = 0; i < PAGEMAP_ENTRY; i++){
        physical_address = (physical_address << 8) + char_buf[i];
    }

    printf("Virtual address: 0x%lx -> Physical address: 0x%llx\n", virt_addr, (unsigned long long) physical_address);

    fclose(f);
}

int main(int argc, char ** argv){
    if(argc != 3){
        printf("Usage: ./lab3 <pid> <virtual address>\n");
        return -1;
    }
    char path[100];

    if(!memcmp(argv[1],"self",sizeof("self"))) sprintf(path, "/proc/self/pagemap");
    else {
        int pid = atoi(argv[1]);
        sprintf(path, "/proc/%u/pagemap", pid);
    }

    unsigned long virtual_address = strtol(argv[2], NULL, 16);

    read_pagemap(path, virtual_address);
    return 0;
}
