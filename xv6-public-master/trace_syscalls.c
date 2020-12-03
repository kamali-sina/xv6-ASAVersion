//code here
#include "types.h"
#include "stat.h"
#include "user.h"

int state = 0;
int
main(int argc, char *argv[]){
    if (argc < 2){
        printf(2,"no state was provided\n");
        exit();
    }
    int state;
    if (argv[1][0] == '1'){
        state = 1;
    }else
        state = 0;

    trace_syscalls(state);
    exit();
}