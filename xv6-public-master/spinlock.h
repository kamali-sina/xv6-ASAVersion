// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  uint pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
};

// Mutual exclusion lock.
struct condvar{
  int first_free;
  struct spinlock lock;
  struct proc* list[20];
  // int locks[20];
};
