#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int count = get_free_pages_count();
  printf(2,"num of free pages: %d \n", count);
  exit();
}