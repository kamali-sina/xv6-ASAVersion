#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(2,"in program\n");
  semaphore_initialize(0,1,0);
  printf(2,"did something\n");
  int pid = fork();
  printf(2,"after fork\n");
  if (pid == 0){
    printf(2,"bache before sem\n");
    semaphore_aquire(0);
    printf(2,"bache\n");
  }else{
    printf(2,"baba before sem\n");
    semaphore_aquire(0);
    printf(2,"baba\n");
  }
  exit();
}