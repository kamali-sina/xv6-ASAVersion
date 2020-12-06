#include "types.h"
#include "stat.h"
#include "user.h"

int drink(int booze){
    return (booze * 10 - booze * 8) % 200000;
}

int
main(int argc, char *argv[])
{
    int booze = 100;
    int shots;
    int drunk = 50;
	for(shots = 0; shots < drunk; shots++) {
        int pid = fork();
        if (pid == 0){
            for (int i = 0; i < 1000000; i++){
                booze = drink(booze);
                exit();
            }
        }
        else{
            for (int i = 0; i < 100000; i++){
                booze = drink(booze);
            }
            wait();
        }
    }
    exit();
}