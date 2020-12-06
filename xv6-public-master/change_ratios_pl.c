#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	if(argc != 5){
		printf(2, "number of argumants is exactly 4 argumants\n");
		exit();
	}
	int result = change_ratios_pl(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));
    if (result == 0){
        printf(2,"did not find pid, terminating!\n");
        exit();
    }
	printf(2,"done\n");
    exit();
}