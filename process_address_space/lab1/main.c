#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int global_initialized = 10;
int global_uninitialized;
const int global_const = 10;

void func(){
    int local = 10;
    const int local_const = 10;
    static int local_static = 10;
    printf("var in function: %p\n", &local);
    printf("const var in function: %p\n", &local_const);
    printf("static var in function: %p\n", &local_static);
}

int* get_pointer(){
    int blabla = 123;
    printf("blabla address: %p\n", &blabla);
    printf("blabla value: %d\n", blabla);
    int* pointer = &blabla;
    return pointer;
}

void hello_world(){
    char* buf = (char*) malloc(100);
    strcpy(buf, "Hello, world!\n");
    printf("buffer contents: %s\n", buf);
    free(buf);
    printf("buffer contents: %s\n", buf);
    char* new_buf = (char*) malloc(100);
    strcpy(new_buf, "Hello, world!\n");
    printf("new buffer contents: %s\n", new_buf);
    //new_buf += 2;
    //free(new_buf);
    //printf("new buffer contents: %s\n", new_buf);
}

void env(){
    char* env_var = getenv("BLABLABLA");
    if(env_var){
        printf("BLABLABLA value is: %s\n", env_var);
    } else printf("BLABLABLA doenst exist");
}

int main(){
    printf("global initialized var: %p\n", &global_initialized);
    printf("global uninitialized var: %p\n", &global_uninitialized);
    printf("global const var: %p\n", &global_const);
    func();
    int* blabla = get_pointer();
    //printf("\n");
    printf("blabla address in main: %p\n", blabla);
    printf("blabla value in main: %d\n", *blabla);
    hello_world();
    return 0;
}