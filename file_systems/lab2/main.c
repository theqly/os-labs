#include "dirent.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

void print_help() {
    printf("Usage:\n"
        "./lab2 create_dir <name> --- creates a new dir\n"
        "./lab2 list <dir_name> --- displays the content of a dir\n"
        "./lab2 remove_dir <dir_name> --- removes an existing dir\n"
        "./lab2 create_file <name> --- creates a new file\n"
        "./lab2 print <file_name> --- prints the content of a file\n"
        "./lab2 remove_file <file_name> --- removes an existing file\n"
        "./lab2 create_symlink <target> <name> --- creates a new symlink\n"
        "./lab2 print_symlink <symlink_name> --- prints the content of a symlink\n"
        "./lab2 print_symlink_file <symlink_name> --- prints the content of a symlink's file\n"
        "./lab2 remove_symlink <symlink_name> --- removes an existing symlink\n"
        "./lab2 create_hardlink <name> --- creates a new hardlink\n"
        "./lab2 remove_hardlink <hardlink_name> --- removes an existing hardlink\n"
        "./lab2 info <file_name> --- displays the mode and number of hardlinks of a file\n"
        "./lab2 change_mode <file_name> --- changes the mode of a file\n");

}

int create_dir(const char* name) {
    if(mkdir(name, 0777) != 0){
        perror("error with creating dir\n");
        return 1;
    }
    return 0;
}

int list(const char* dir_name) {
    DIR* dir;
    struct dirent* entry;
    if((dir = opendir(dir_name)) == NULL){
        perror("error with opening dir for list\n");
        return 1;
    }
    while((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    closedir(dir);
    return 0;
}

int remove_dir(const char* dir_name) {
    if(rmdir(dir_name) != 0) {
        perror("error with removing dir\n");
        return 1;
    }
    return 0;
}

int create_file(const char* name) {
    const int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("error with creating file\n");
        return 1;
    }
    close(fd);
    return 0;
}

int print(const char* file_name) {
    int fd = open(file_name, O_RDONLY);
    if(fd == -1) {
        perror("error with opening file\n");
        return 1;
    }

    char buf[256];
    ssize_t bytes_read;
    while((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        if(write(STDOUT_FILENO, buf, bytes_read) != bytes_read) {
            perror("error with printing file\n");
            return 1;
        }
    }

    if(bytes_read == -1) {
        perror("error with reading file\n");
        return 1;
    }

    close(fd);
    return 0;
}

int remove_file(const char* file_name) {
    if(remove(file_name) != 0) {
        perror("error with removing dir\n");
        return 1;
    }
    return 0;
}

int create_symlink(const char* target, const char* name) {
    if(symlink(target, name) != 0) {
        perror("error with creating symlink\n");
        return 1;
    }
    return 0;
}

int print_symlink(const char* symlink_name) {
    char buf[PATH_MAX];
    ssize_t bytes_read = readlink(symlink_name, buf, sizeof(buf));
    if(bytes_read == -1) {
        perror("error with reading symlink");
        return 1;
    }
    buf[bytes_read] = '\0';
    printf("%s\n", buf);
    return 0;
}

int print_symlink_file(const char* symlink_name) {
    char buf[PATH_MAX];
    ssize_t bytes_read = readlink(symlink_name, buf, sizeof(buf));
    if(bytes_read == -1) {
        perror("error with reading symlink");
        return 1;
    }
    buf[bytes_read] = '\0';
    print(buf);
}

int remove_symlink(const char* symlink_name) {
    if(unlink(symlink_name) != 0) {
        perror("error with unlinking symlink\n");
        return 1;
    }
    return 0;
}

int create_hardlink(const char* target, const char* name) {
    if(link(target, name) != 0) {
        perror("error with creating hardlink");
        return 1;
    }
    return 0;
}

int remove_hardlink(const char* hardlink_name) {
    if(unlink(hardlink_name) != 0) {
        perror("error with unlinking hardlink");
        return 1;
    }
    return 0;
}

void convert(mode_t* mode, char* str, int to_mode) {
    if(to_mode) {
        for(int i = 0; i < strlen(str); ++i) {
            if(str[i] == 'r') *mode |= S_IRUSR | S_IRGRP | S_IROTH;
            else if(str[i] == 'w') *mode |= S_IWUSR | S_IWGRP | S_IWOTH;
            else if(str[i] == 'x') *mode |= S_IXUSR | S_IXGRP | S_IXOTH;
            else if(str[i] == '-') continue;
            else {
                fprintf(stderr, "invalid mode symbol\n");
                return;
            }
        }
    } else {
        for(int i = 0; i < 9; ++i) {
            str[i] = '-';
        }
        str[9] = '\0';

        if (S_ISDIR(*mode)) str[0] = 'd';
        else if (S_ISLNK(*mode)) str[0] = 'l';

        if ((*mode & S_IRUSR) != 0) str[1] = 'r';
        if ((*mode & S_IWUSR) != 0) str[2] = 'w';
        if ((*mode & S_IXUSR) != 0) str[3] = 'x';
        if ((*mode & S_IRGRP) != 0) str[4] = 'r';
        if ((*mode & S_IWGRP) != 0) str[5] = 'w';
        if ((*mode & S_IXGRP) != 0) str[6] = 'x';
        if ((*mode & S_IROTH) != 0) str[7] = 'r';
        if ((*mode & S_IWOTH) != 0) str[8] = 'w';
        if ((*mode & S_IXOTH) != 0) str[9] = 'x';
    }
}

int info(const char* file_name) {
    struct stat info;
    if(stat(file_name, &info) != 0) {
        perror("error with getting info");
        return 1;
    }
    char* mode = (char*)malloc(sizeof(char) * 10);
    convert(&info.st_mode, mode, 0);
    printf("Mode: %s\n", mode);
    printf("Num of hardlinks: %lu\n", info.st_nlink);
    return 0;
}

int change_mode(const char* file_name, const char* new_mode) {
    char tmp[10];
    strcpy(tmp, new_mode);
    mode_t mode = 0;
    convert(&mode, tmp, 1);
    if(chmod(file_name, mode) != 0) {
        perror("error with changing mode");
        return 1;
    }
    return 0;
}

int work(int argc, char** argv) {
    if(strcmp(argv[1], "help") == 0) print_help();

    else if(strcmp(argv[1], "create_dir") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return create_dir(argv[2]);
    }

    else if(strcmp(argv[1], "list") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return list(argv[2]);
    }

    else if(strcmp(argv[1], "remove_dir") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return remove_dir(argv[2]);
    }

    else if(strcmp(argv[1], "create_file") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return create_file(argv[2]);
    }

    else if(strcmp(argv[1], "print") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return print(argv[2]);
    }

    else if(strcmp(argv[1], "remove_file") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return remove_file(argv[2]);
    }

    else if(strcmp(argv[1], "create_symlink") == 0) {
        if(argc != 4) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return create_symlink(argv[2], argv[3]);
    }

    else if(strcmp(argv[1], "print_symlink") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return print_symlink(argv[2]);
    }

    else if(strcmp(argv[1], "print_symlink_file") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return print_symlink_file(argv[2]);
    }

    else if(strcmp(argv[1], "remove_symlink") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return remove_symlink(argv[2]);
    }

    else if(strcmp(argv[1], "create_hardlink") == 0) {
        if(argc != 4) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return create_hardlink(argv[2], argv[3]);
    }

    else if(strcmp(argv[1], "remove_hardlink") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return remove_hardlink(argv[2]);
    }

    else if(strcmp(argv[1], "info") == 0) {
        if(argc != 3) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return info(argv[2]);
    }

    else if(strcmp(argv[1], "change_mode") == 0) {
        if(argc != 4) {
            fprintf(stderr, "wrong number of args\n");
            return 1;
        }
        return change_mode(argv[2], argv[3]);
    }

    return 0;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        fprintf(stderr, "wrong number of args\n");
        return 1;
    }
    //было бы неплохо фабрику конечно, но не в этой жизни
    if(work(argc, argv) != 0) {
        fprintf(stderr, "something went wrong\n");
        return 1;
    }

    return 0;
}