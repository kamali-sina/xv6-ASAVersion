//code here
#include "types.h"
#include "stat.h"
#include "user.h"

int no_loop[10];

void print_children(int pid){
  int temp = get_children(pid);
  no_loop[pid] = 1;
  if(temp < 0){
    printf(1, "It has no 1 digit pid children\n");
    return;
  }
  printf(1, "children: %d\n", temp);
  while(temp > 0){
    if (no_loop[temp % 10] == 0)
      print_children(temp % 10);
    temp /= 10;
  }
}

int
main(int argc, char *argv[]){
  for (int i = 0 ; i < 10 ; i++)
    no_loop[i] = 0;
  int pid = getpid();
  int a = fork();
  int b = fork();
  if(a == 0 && b != 0){
    int d = fork();
    if(d == 0){
      while(1);
    }
    while(1);
  }
  if(b == 0 && a != 0){
    while(1);
  }
  if(a != 0 && b != 0){
    print_children(pid);
  }
  sleep(1);
  exit();
}