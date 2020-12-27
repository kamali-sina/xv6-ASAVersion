// #include "types.h"
// #include "user.h"
// #include "spinlock.h"

// int lock(struct spinlock* lk){
//     pushcli(); // disable interrupts to avoid deadlock.
//     if(holding(lk))
//         panic("acquire");

//     // The xchg is atomic.
//     while(xchg(&lk->locked, 1) != 0)
//         ;

//     // Tell the C compiler and the processor to not move loads or stores
//     // past this point, to ensure that the critical section's memory
//     // references happen after the lock is acquired.
//     __sync_synchronize();

//     // Record info about lock acquisition for debugging.
//     // lk->cpu = mycpu();
//     // getcallerpcs(&lk, lk->pcs);


//     // while(loc->locked != 0)
//     // ;
//     // loc->locked = 1;
//     return 0;
// }

// int init_lock(struct spinlock* lk){
//     lk->locked = 0;
//     lk->locked = 0;
//     lk->cpu = 0;
//     return 0;
// }

// int unlock(struct spinlock* lk){
//     if(!holding(lk))
//     panic("release");
//     // Tell the C compiler and the processor to not move loads or stores
//     // past this point, to ensure that all the stores in the critical
//     // section are visible to other cores before the lock is released.
//     // Both the C compiler and the hardware may re-order loads and
//     // stores; __sync_synchronize() tells them both not to.
//     __sync_synchronize();

//     // Release the lock, equivalent to lk->locked = 0.
//     // This code can't use a C assignment, since it might
//     // not be atomic. A real OS would use C atomics here.
//     asm volatile("movl $0, %0" : "+m" (lk->locked) : );

//     popcli();
//     //loc->locked = 0;
//     return 0;
// }