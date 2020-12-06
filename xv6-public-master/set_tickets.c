#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{

	if(argc != 3){
		printf(2, "number of argumants is exactly 2 argumants\n");
		exit();
	}
	int result = set_tickets(atoi(argv[1]),atoi(argv[2]));
    if (result == 0){
        printf(2,"did not find pid, terminating!\n");
        exit();
    }
	printf(2,"done\n");
    exit();
}