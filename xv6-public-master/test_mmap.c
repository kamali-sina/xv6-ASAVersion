#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int count = get_free_pages_count();
  printf(2,"num of free pages: %d \n", count);
  
  // printf(2,"mmap is in the works! \n");
  // lcm();
  int fd = open("test_free.c", 0x001 | 0x200);
  mmap(0, 2, 0, 0, fd, 0);
  count = get_free_pages_count();
  printf(2,"num of free pages: %d \n", count);
  close(fd);
  exit();
}