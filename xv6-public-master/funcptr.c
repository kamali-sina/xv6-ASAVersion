#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    int (*funcptr)(void) = &getpid;
    printf(1, "%d\n", (*funcptr)());
    exit();
}