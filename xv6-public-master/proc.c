#include "types.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#define MAX_WAITING_TIME 10000
#define NULL 0

int system_priority_ratio = 1;
int system_arrival_time_ratio = 1;
int system_executed_cycle_ratio = 1;

int trace_state  = 0;
int do_once = 1;
proc_trace tracer[100];
static char* list[] = {
    "fork",
    "exit",
    "wait",
    "pipe",
    "read",
    "kill",
    "exec",
    "fstat",
    "chdir",
    "dup",
    "getpid",
    "sbrk",
    "sleep",
    "uptime",
    "open",
    "write",
    "mknod",
    "unlink",
    "link",
    "mkdir",
    "close",
    "reverse_number",
    "trace_syscalls",
    "get_children",
    "setup_trace",
    "level_change",
    "set_tickets",
    "change_ratios_pl",
    "change_ratios_sl",
    "htop",
    0
};

int reset_trace(){
  for (int i = 0; i < 100; i++){
    tracer[i].valid = 0;
    for (int j = 0 ; j < 27 ; j++){
      tracer[i].syscalls[j] = 0;
    }
  }
  return 0;
}

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  //giving value to new fileds
  //TODO: double check
  p->tickets = 10;
  struct rtcdate t1;
  cmostime(&t1);
  int time = 0;
  time += t1.second;
  time += t1.minute * 100;
  time += t1.hour * 10000;
  p->arrival_time = time;
  p->executed_cycle = 0;
  p->arrival_time_ratio = system_arrival_time_ratio;
  p->priority_ratio = system_priority_ratio;
  p->executed_cycle_ratio = system_executed_cycle_ratio;
  p->level = 1;
  p->waited = 0;
  //end of costume values
  release(&ptable.lock);
  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  reset_trace();
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);
  //giving value to new fileds
  //TODO: double check
  np->tickets = 1;
  struct rtcdate t1;
  cmostime(&t1);
  int time = 0;
  time += t1.second;
  time += t1.minute * 100;
  time += t1.hour * 10000;
  np->arrival_time = time;
  np->executed_cycle = 0;
  np->level = 2;
  np->waited = 0;
  np->arrival_time_ratio = system_arrival_time_ratio;
  np->priority_ratio = system_priority_ratio;
  np->executed_cycle_ratio = system_executed_cycle_ratio;
  //end of costume values
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void agging(struct proc *p){
  level_change(p->pid, 1);
  p->waited = 0;
}

int level_finder(struct cpu *c){
  struct proc *p;
  int level = 3;
  int flag = 0;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE){
      continue;
    }
    flag = 1;
    p->waited++;
    if(p->waited >= MAX_WAITING_TIME)
      agging(p);

    if(p->level == 1){
      level = 1;
    }

    if(p->level == 2 && level != 1)
      level = 2;
  }
  release(&ptable.lock);
  if(flag == 0)
    return 4;
  return level;
}

void 
round_robin(struct cpu *c){
  struct proc *p;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 1)
      continue;

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    p->waited--;
    p->executed_cycle += 0.1;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
  }
  release(&ptable.lock);
}

struct proc *find_best_job(struct cpu *c){
  struct proc *p;
  struct proc *min_rank = NULL;
  float min_rank_value = -1;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 3)
      continue;
    
    float rank = (p->priority_ratio / p->tickets) + (p->arrival_time * p->arrival_time_ratio) + (p->executed_cycle * p->executed_cycle_ratio);
    if(min_rank_value == -1 || min_rank_value > rank){
      min_rank_value = rank;
      min_rank = p;
    }
  }
  release(&ptable.lock); 
  return min_rank;
}

void BJF(struct cpu *c){
  struct proc *best_job = find_best_job(c);
  struct proc *p;
  if(best_job == NULL){
    cprintf("fuck------------------------\n");
    return;
  }
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 1)
      continue;

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    if(best_job != p)
      continue;
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    p->waited--;
    p->executed_cycle += 0.1;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
  }
  release(&ptable.lock);
}

int random_number(int mod){
  struct rtcdate t1;
  cmostime(&t1);
  int time = 0;
  time += t1.second;
  time += t1.minute * 100;
  time += t1.hour * 10000;
  unsigned int next = time;
  int result;

  next *= 1103515245;
  next += 12345;
  result = (unsigned int) (next / 65536) % 2048;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  return result % mod + 1;
}

struct proc * find_winner(struct cpu *c){
  struct proc *p;
  int mod = 0;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 2)
      continue;
      
    mod += p->tickets;
  }
  int winner_number = random_number(mod);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 2)
      continue;
      
    winner_number -= p->tickets;
    if(winner_number <= 0){
      release(&ptable.lock);
      return p;
    }
  }
  release(&ptable.lock);   
  return NULL;
}

struct proc *find_with_pid(struct cpu *c, int pid){
  struct proc *p;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // if(p->state != RUNNABLE || p->level != 2)
    //   continue;
    if(p->pid == pid){
      release(&ptable.lock);  
      return p;
    }
  }
  release(&ptable.lock);   
  return NULL;
}

int lottery(struct cpu *c, int pid){
  // cprintf("In lottery\n");
  struct proc *winner_proc = find_with_pid(c, pid);
  if(winner_proc == NULL){
    winner_proc = find_winner(c);
  }
  // p = find_winner(c);
  if(winner_proc == NULL)
    return -123;
  
  struct proc *p;
  // Loop over process table looking for process to run.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 1)
      continue;

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    if(p != winner_proc)
      continue;

    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    p->waited--;
    p->executed_cycle += 0.1;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
  }
  release(&ptable.lock);
  return -123;
}

void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
  int pid = -123;
  for(;;){
    // Enable interrupts on this processor.
    sti();
    int level = level_finder(c);
    switch(level){
      case 1: 
        round_robin(c);
        break;
      case 2: 
        pid = lottery(c, pid);
        // cprintf("pid:  %d\n", pid);
        break;
      case 3: BJF(c);
        break;
      default: break;
    }
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

//costume syscalls

int reverse_number(int num){
  int result = 0;
  while(num > 0) {
    result *= 10;
    result += num % 10;
    num /= 10;
  }
  return result;
}


int trace_syscalls(int state){
  argint(0,&state);
  trace_state = state;
  if (state == 0)
    reset_trace();
  cprintf("state is now %d\n", trace_state);
  return 0;
}

int get_children(int pid){
  struct proc *p;
  int found = 0;
  int childern_pids = 0;
  int mult = 10;
  argint(0, &pid);
  cprintf("PID: %d\n", pid);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent->pid == pid){
      if (mult <= pid)
        continue;
      childern_pids *= mult;
      childern_pids += p->pid;
      found = 1;
    }
  }
  if(found == 1)
    return childern_pids;
  return -1;
}

void print_trace(){
  cprintf("\n");
  for (int i = 0 ; i < 100 ; i ++){
    if (tracer[i].valid == 1){
      cprintf("%s\n",tracer[i].name);
      for (int j = 0 ; j < 27 ; j++){
        if (tracer[i].syscalls[j] > 0)
          cprintf("          %s: %d\n",list[j-1] , tracer[i].syscalls[j]);
      }
      cprintf("\n");
    }
    else break;
  }
  cprintf("$ ");
}

int trace_handler(){
  int lasttick = ticks/100;
  int time_passed = 5;
  int checker = 1;
  while (1){
    if (checker > 0){
      cprintf("starting\n");
      checker--;
    }
    if (trace_state != 0){
      if (lasttick != ticks/100){
        time_passed++;
        lasttick = ticks/100;
      }
      if (trace_state == 1 && time_passed > 6){
        time_passed = 0;
        if (trace_state == 1){
          cprintf("printing all:\n");
          print_trace();
        }
      }
    }
    myproc()->state = RUNNING;
  }
  return 0;
}

void copy_proc(struct proc *src, struct proc *dest){
  dest->sz = dest->sz;                     // Size of process memory (bytes)
  dest->pgdir = src->pgdir;                // Page table
  dest->kstack = src->kstack;                // Bottom of kernel stack for this process
  dest->state = src->state;        // Process state
  dest->pid = src->pid;                     // Process ID
  dest->parent = src->parent;         // Parent process
  dest->tf = src->tf;        // Trap frame for current syscall
  dest->context = src->context;     // swtch() here to run process
  dest->chan = src->chan;                  // If non-zero, sleeping on chan
  dest->killed = src->killed;                  // If non-zero, have been killed
  int i;
  for(i = 0; i < NOFILE; i++)
    dest->ofile[i] = src->ofile[i];  // Open files
  dest->cwd = src->cwd;           // Current directory
  for(i = 0; i < 16; i++)
    dest->name[i] = src->name[i];               // Process name (debugging)

  dest->tickets = src->tickets;                 // number of tickets for lottory, priority of process is 1/ticket number
  dest->arrival_time = src->arrival_time;             // time wich process has arrived
  dest->executed_cycle = src->executed_cycle;        // time wich process was running
  dest->priority_ratio = src->priority_ratio;
  dest->arrival_time_ratio = src->arrival_time_ratio;
  dest->executed_cycle_ratio = src->executed_cycle_ratio;
  dest->level = src->level;
  dest->waited = src->waited;
  //TODO: waited 
}

int setup_trace(int dummy){
  return trace_handler();
}

int level_change(int pid, int level){
  struct proc *procNeeded;
  struct proc *p;
  procNeeded = NULL;
  // acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid == pid){
      procNeeded = p;
    }
  }
  if (procNeeded == NULL || (level <= 0 && level >= 4)){
    release(&ptable.lock);
    return 0;
  }
  procNeeded->level = level;
  // copy
  // struct proc temp;
  // copy_proc(procNeeded, &temp);
  // for(p = procNeeded+1; p < &ptable.proc[NPROC]; p++){
  //   if(p->state == UNUSED){
  //     copy_proc(&temp, p);
  //     break;
  //   }
  //   else{
  //     copy_proc(p, p-1);
  //   }
  // }
  // release(&ptable.lock);
  return 1;
}

int set_tickets(int pid, int tickets){
  struct proc *procNeeded;
  struct proc *p;
  procNeeded = NULL;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid == pid){
      procNeeded = p;
    }
  }
  if (procNeeded == NULL){
    release(&ptable.lock);
    return 0;
  }
  procNeeded->tickets = tickets;
  release(&ptable.lock);
  return 1;
}

int change_ratios_pl(int pid, int priority_ratio, int arrival_time_ratio, int ec_ration){
  struct proc *procNeeded;
  struct proc *p;
  procNeeded = NULL;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid == pid){
      procNeeded = p;
    }
  }
  if (procNeeded == NULL){
    release(&ptable.lock);
    return 0;
  }
  procNeeded->priority_ratio = priority_ratio;
  procNeeded->arrival_time_ratio = arrival_time_ratio;
  procNeeded->executed_cycle_ratio = ec_ration;
  release(&ptable.lock);
  return 1;
}

int change_ratios_sl(int pid, int priority_ratio, int arrival_time_ratio, int ec_ration){
  system_priority_ratio = priority_ratio;
  system_executed_cycle_ratio = ec_ration;
  system_arrival_time_ratio = arrival_time_ratio;
  return 1;
}

void htop(){
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  cprintf("name                 pid       state            tickets\n-------------------------------------------------------\n");
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      break;
    if (p != NULL){
      int i;
      cprintf("%s",p->name);
      for (i = 0; i < 20 - strlen(p->name) ; i++){
        cprintf(" ");
      }
      cprintf(" %d", p->pid);
      int n = p->pid;
      int count = 0;
      while (n != 0) 
      {
          n = n / 10;
          ++count;
      }
      for (i = 0; i < 10 - count ; i++){
        cprintf(" ");
      }
      cprintf("%s              %d", states[p->state], p->tickets);
      cprintf("\n");
    }
  }
  release(&ptable.lock);
}
