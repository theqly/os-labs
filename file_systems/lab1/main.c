#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "dirent.h"

void reverse(char *str) {
    for(int i = 0, j = strlen(str) - 1; i < j; ++i, --j){
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

void reverse_file(FILE* src, FILE* dst){
    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    fseek(src, 0, SEEK_SET);
    char* buff = (char*) malloc(size);
    fread(buff, size, 1, src);
    for(int i = size - 1; i >= 0; --i){
        fwrite(&buff[i], sizeof(char), 1, dst);
    }
    free(buff);
}

int main(int argc, char** argv){
    if (argc != 2){
        fprintf(stderr, "wrong number of args\n");
        return 1;
    }

    char* src_dir_name = argv[1];
    char dst_dir_name[strlen(src_dir_name)];
    strcpy(dst_dir_name, src_dir_name);
    reverse(dst_dir_name);
    if(mkdir(dst_dir_name, 0777) != 0){
        perror("error with creating dir\n");
        return 1;
    }

    DIR* dir;
    struct dirent* cur_file;
    if((dir = opendir(src_dir_name)) == NULL){
        perror("error with opening dir\n");
        return 1;
    }

    while((cur_file = readdir(dir)) != NULL){
        if (cur_file->d_type == DT_REG) {
            char* name = cur_file->d_name;
            char src_path[strlen(src_dir_name) + strlen(name) + 2];
            sprintf(src_path, "%s/%s", src_dir_name, name);
            reverse(name);
            char dst_path[strlen(dst_dir_name) + strlen(name) + 2];
            sprintf(dst_path, "%s/%s", dst_dir_name, name);

            //reverse(dst_path);
            printf("56 %s\n", src_path);
            printf("57 %s\n", dst_path);
            FILE *src_file = fopen(src_path, "rb");
            FILE *dst_file = fopen(dst_path, "wb");

            if (src_file && dst_file) {
                reverse_file(src_file, dst_file);
                fclose(src_file);
                fclose(dst_file);
            } else {
                perror("error with opening file");
            }
        }
    }
    closedir(dir);
    return 0;
}