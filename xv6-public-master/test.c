#include "usr_lock.h"

int main(){
    // struct condvar* condition = (struct condvar*)malloc(sizeof(struct condvar));
    // cv_init(condition);
    // init_lock(condition->lock);
    // struct spinlock *dodol = (struct spinlock*)malloc(sizeof(struct spinlock));
    // init_lock(dodol);
    // lock(dodol);
    // semaphore_initialize(0,0,0);
    int pid = fork();
    if(pid < 0){
        printf(1, "Error forking first child.\n");
    }
    else{
        if(pid == 0){
            printf(1, "Child 1 executing.\n");
            // semaphore_release(0);
            // cv_signal(condition);
            // unlock(dodol);
        }
        else{
            pid = fork();
            if(pid < 0){
                printf(1, "Error forking second child.\n");
            }
            else{
                if(pid == 0){
                    // cv_wait(condition);
                    // lock(dodol);
                    // semaphore_aquire(0);
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