#include "stdio.h"

int main(){
	asm (
		"mov $1, %%rax\n"
		"mov $1, %%rdi\n"
		"mov %[msg], %%rsi\n"
		"mov $12, %%rdx\n"
		"syscall\n"
		:
		: [msg] "r" ("Hello World\n")
		: "%rax", "%rdi", "%rsi", "%rdx"
	);
	return 0;
}

