#include "types.h"
#include "user.h"
#include "spinlock.h"

int lock(struct spinlock* loc){
    while(loc->locked != 0)
    ;
    loc->locked = 1;
    return 0;
}

int init_lock(struct spinlock* loc){
    loc = (struct spinlock*)malloc(sizeof(struct spinlock)); 
    loc->locked = 0;
    return 0;
}

int unlock(struct spinlock* loc){
    loc->locked = 0;
    return 0;
}