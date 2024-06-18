#include <pwd.h>
#include <stdio.h>
#include <unistd.h>

int file_test() {
    FILE *file = fopen("../filesystem/system", "r");
    if(!file) {
        perror("error in fopen");
        return 1;
    }
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
    printf("\n");
    fclose(file);
    return 0;
}

void print_user_ids() {
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

int main(void) {
    file_test();
    print_user_ids();
    return 0;
}
