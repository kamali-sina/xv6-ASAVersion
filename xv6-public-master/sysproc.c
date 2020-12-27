#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
int 
sys_semaphore_initialize(int sid, int v, int m)
{
  semaphore_initialize(sid,v,m);
  return 0;
}

int 
sys_semaphore_aquire(int sid)
{
  return semaphore_aquire(sid);
}

int 
sys_semaphore_release(int sid)
{
  return semaphore_release(sid);
}

//condtion

struct condvar* sys_cv_init(){
  return cv_init();
}

int sys_cv_wait(struct condvar* condition)
{
  cv_wait(condition);
  return 0;
}

int sys_cv_signal(struct condvar* condition)
{
  cv_signal(condition);
  return 0;
}

int sys_amu_wait(struct condvar* condition)
{
  amu_wait(condition);
  return 0;
}

int sys_amu_signal(struct condvar* condition)
{
  amu_signal(condition);
  return 0;
}

int sys_amu_inc(struct condvar* condition){
  return amu_inc(condition);
}

int sys_amu_dec(struct condvar* condition){
  return amu_dec(condition);
}

int sys_amu_get(struct condvar* condition){
  return amu_get(condition);
}