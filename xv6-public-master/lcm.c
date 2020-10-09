#include "types.h"
#include "stat.h"
#include "user.h"

int lcm_of_two (int num1, int num2){
	int max_of_two = (num1 > num2)? num1 : num2;
	while (1){
		if(max_of_two % num1 == 0 && max_of_two % num2 == 0){
    	return max_of_two;
		}
		++max_of_two;
	}
}

char* itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}



int
main(int argc, char *argv[])
{

  if(argc < 3){
    printf(2, "lcm needs at least 2 argumants\n");
    exit();
  }
	if (argc > 9){
		printf(2, "The maximum number of arguments is 8\n");
    exit();
	}
	int lcm = lcm_of_two(atoi(argv[1]),atoi(argv[2]));
	for (int i = 3; i<argc; i++){
		lcm = lcm_of_two(atoi(argv[i]), lcm);
	}

	int fd = open("lcm_result.txt",0x001 | 0x200); 
	write(fd, itoa(lcm,10),sizeof(itoa(lcm,10)));
	write(fd,"\n",1);
	close(fd);
  exit();
}