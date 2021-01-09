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
	set_tickets(atoi(argv[1]),atoi(argv[2]));
	printf(2,"done\n");
    exit();
}