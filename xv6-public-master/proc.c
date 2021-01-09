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
#define NNULL 1

int system_priority_ratio = 1;
int system_arrival_time_ratio = 1;
int system_executed_cycle_ratio = 1;
int random_number = 6985420;

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
  p->lottery_tickets = 100;
  p->queue_priority = 3;
  p->arrival_time = ticks;
  p->exec_cycles = 0;
  p->priority = (float)(1 / (float)p->lottery_tickets);
  p->arrival_time_ratio = system_arrival_time_ratio;
  p->exec_cycles_ratio = system_executed_cycle_ratio;
  p->priority_ratio = system_priority_ratio;
  int rank = (p->arrival_time * p->arrival_time_ratio) + (p->exec_cycles * p->exec_cycles_ratio) + (p->priority * p->priority_ratio);
  p->rank = rank;
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
void
aging(int pid){
  for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state != UNUSED && p->pid != pid)
      if(++p->cycles_waited >= MAX_WAITING_TIME)
        level_change(p->pid, 2);
}

void
run_process(struct cpu* mycpu, struct proc* p){
  if(p->state == RUNNABLE)
  {
    p->exec_cycles += 1;
    mycpu->proc = p;
    switchuvm(p);
    p->state = RUNNING;
    swtch(&(mycpu->scheduler), p->context);
    switchkvm();
    mycpu->proc = 0;
    int pid = p->pid;
    aging(pid);
  }
}

void
round_robin_scheduler(){
  // Copy paste
  struct proc* p;
  struct cpu *c = mycpu();
  c->proc = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == RUNNABLE && p->queue_priority == 1)
      run_process(c,p);
}

unsigned int
rand(){
  random_number = random_number * 918251 + 3215654221;
  return random_number;
}

int
generate_random_number(int min, int max){
  return rand() % (max - min + 1) + min;
  // return ticks % (max - min + 1) + min;
}

void
lottery_scheduler(){
  struct cpu *c = mycpu();
  int total = 0;
  int sigma = 0;

  for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->queue_priority == 2)
      total += p->lottery_tickets;
  }
  int random = generate_random_number(1,total);
  for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->queue_priority == 2){
      sigma += p->lottery_tickets;
      if (sigma >= random){
        run_process(c,p);
        break;
      }
    }
  }
}

void bjf_scheduler(){
  int min = 2000000000;
  int id = 0;
  struct cpu *c = mycpu();
  for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->queue_priority == 3){
      int p_rank = (p->arrival_time * p->arrival_time_ratio) + (p->exec_cycles * p->exec_cycles_ratio) + (p->priority * p->priority_ratio); 
      if(min > p_rank){
        min = p_rank;
        id = p->pid;
      }
    }
  }
  for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->queue_priority == 3 && p->pid == id){
      run_process(c,p);
      break;
    }
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
void
scheduler(void){
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){

    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE){
        switch(p->queue_priority){
          case 1:
          {
            round_robin_scheduler();
            break;
          }
          case 2:
          {
            lottery_scheduler();
            break;
          }
          case 3:
          {
            bjf_scheduler();
            break;
          }
        }
      }
    }

    release(&ptable.lock);
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

int setup_trace(int dummy){
  return trace_handler();
}

int level_change(int pid, int level){
  argint(0,&pid);
  argint(1,&level);
  struct proc *p;
  int yes = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(pid == p->pid){
      p->queue_priority = level;
      yes = 1;
      break;
    }
  }
  if (!yes)
    return -1;
  return 0;
}

int set_tickets(int pid, int tickets){
  argint(0,&pid);
  argint(1,&tickets);
  struct proc *p;
  int yes = 0;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid == pid){
      p->lottery_tickets = tickets;
      yes = 1;
      break;
    }
  }
  if (!yes)
    return -1;
  return 0;
}

int change_ratios_pl(int pid, int priority_ratio, int arrival_time_ratio, int ec_ration){
  argint(0, &pid);
  argint(1, &priority_ratio);
  argint(2, &arrival_time_ratio);
  argint(3, &ec_ration);
  int yes = 0;
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid == pid){
      p->arrival_time_ratio = arrival_time_ratio;
      p->exec_cycles_ratio = ec_ration;
      p->priority_ratio = priority_ratio;
      yes = 1;
      int rank = (p->arrival_time * p->arrival_time_ratio) + (p->exec_cycles * p->exec_cycles_ratio) + (p->priority * p->priority_ratio);
      p->rank = rank;
      break;
    }
  }
  if (!yes)
    return -1;
  return 0;
}

int change_ratios_sl(int pid, int priority_ratio, int arrival_time_ratio, int ec_ration){
  argint(0, &pid);
  argint(1, &priority_ratio);
  argint(2, &arrival_time_ratio);
  argint(3, &ec_ration);
  system_priority_ratio = priority_ratio;
  system_executed_cycle_ratio = ec_ration;
  system_arrival_time_ratio = arrival_time_ratio;
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p->pid > 0){
      p->arrival_time_ratio = arrival_time_ratio;
      p->exec_cycles_ratio = ec_ration;
      p->priority_ratio = priority_ratio;
      int rank = (p->arrival_time * p->arrival_time_ratio) + (p->exec_cycles * p->exec_cycles_ratio) + (p->priority * p->priority_ratio);
      p->rank = rank;
    }
  }
  return 0;
}

int getlen(int n){
  int count = 0;
  while (n != 0){
    n = n / 10;
    ++count;
  }
  return n;
}

void print_spaces(int spaces, int count){
  for (int i = 0; i < spaces - count ; i++){
    cprintf(" ");
  }
}

void htop(){
  static char *states[] = {
    [UNUSED] "unused",
    [EMBRYO] "embryo",
    [SLEEPING] "sleep ",
    [RUNNABLE] "runble",
    [RUNNING] "run",
    [ZOMBIE] "zombie"
  };

  struct proc *p;
  cprintf("name                 pid       state            tickets           level     executedC       ratio\n------------------------------------------------------------------------------------------------------\n");
  acquire(&ptable.lock);
  int Sstate = 17;
  int Stickets = 13;
  int Slevel = 10;
  int SexecutedC = 16;
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
      int count = getlen(n);
      print_spaces(10, count);

      cprintf("%s", states[p->state]);
      count = strlen(states[p->state]);
      print_spaces(Sstate, count);

      cprintf("%d", p->lottery_tickets);
      count = getlen(p->lottery_tickets);
      print_spaces(Stickets, count);

      cprintf("%d",p->queue_priority);
      count = getlen(p->queue_priority);
      print_spaces(Slevel, count);

      cprintf("%d",p->exec_cycles);
      count = getlen(p->exec_cycles);
      print_spaces(SexecutedC, count);

      cprintf("%d, %d, %d", p->priority_ratio, p->arrival_time_ratio, p->exec_cycles_ratio);
      cprintf("\n");
    }
  }
  release(&ptable.lock);
}
