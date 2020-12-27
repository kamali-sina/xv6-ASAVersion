#include "usr_lock.h"

int main(){
    struct condvar* condition = (struct condvar*)malloc(sizeof(struct condvar));
    cv_init(condition);
    init_lock(&condition->lock);
    int pid = fork();
    if(pid < 0){
        printf(1, "Error forking first child.\n");
    }
    else{
        if(pid == 0){
            printf(1, "Child 1 executing.\n");
            lock(&condition->lock);
            cv_signal(condition);
            unlock(&condition->lock);
        }
        else{
            pid = fork();
            if(pid < 0){
                printf(1, "Error forking second child.\n");
            }
            else{
                if(pid == 0){
                    lock(&condition->lock);
                    cv_wait(condition);
                    unlock(&condition->lock);
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