#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	if(argc != 4){
		printf(2, "number of argumants is exactly 3 argumants\n");
		exit();
	}
	change_ratios_sl(0,atoi(argv[1]),atoi(argv[2]),atoi(argv[3]));
	printf(2,"done\n");
    exit();
}