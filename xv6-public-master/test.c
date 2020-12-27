#include "types.h"
#include "user.h"

int main(){
    struct condvar* condition = (struct condvar*)malloc(sizeof(struct condvar));
    condition = cv_init();
    int pid = fork();
    if(pid < 0){
        printf(1, "Error forking first child.\n");
    }
    else{
        if(pid == 0){
            
            printf(1, "Child 1 executing.\n");
            cv_signal(condition);
        }
        else{
            pid = fork();
            if(pid < 0){
                printf(1, "Error forking second child.\n");
            }
            else{
                if(pid == 0){
                    cv_wait(condition);
                    printf(1, "Child 2 executing.\n");
                    
                }
                else{
                    int i;
                    for(i = 0; i <2;i++){
                        wait();
                    }
                }
            }
        }
    }
    exit();
}