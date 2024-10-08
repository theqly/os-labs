#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>

void wrapper(int(*fn)(void* arg)) {
    fn(NULL);

}

void my_thread_create(int(*fn)(void* arg)) {

    //mmap stack
    //get args

    //clone(wrapper, )
}



int main(void)
{
    printf("Hello, World!\n");
    return 0;
}
