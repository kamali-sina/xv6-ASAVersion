//code here
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]){
    if(argc < 2) {
		printf(1,"not enough arguments!\n");
		exit();
	}
    int prev;
	int num = atoi(argv[1]);
	asm volatile(
		"movl %%ebx, %0;"
		"movl %1, %%ebx;"
		: "=r" (prev)
		: "r"(num)
		: "%ebx"
	);
	printf(1, "%d\n", reverse_number());
	asm volatile(
		"movl %0, %%ebx"
		:
		: "r"(prev)
	);
	exit();
}