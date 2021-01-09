// #include "types.h"
// #include "stat.h"
// #include "user.h"
// #include "fcntl.h"

// #define NCHILDPROC 20

// int
// main(int argc, char *argv[])
// {
//     int is_parent_proc = 0;
//     int is_child_proc = 0;
//     for (int i = 0; i < NCHILDPROC; i++){
//         int pid = fork();
//         if (pid == 0){
//             is_child_proc = 1;
//             break;
//         } else if (pid > 0){
//             printf(1,"A child process with pid: %d has been created.\n", pid);
//             level_change(pid, i < NCHILDPROC / 2 ? 2 : 3);
//             is_parent_proc = 1;
//         }
//     }
//     if (is_child_proc){
//         is_parent_proc = 0;
//         int ex = 0;
//         for (int i = 0; i < 10000 * getpid(); i++){
//             ex += i;
//             ex -= i/2;
//         }
//         exit();
//     }
//     if (is_parent_proc)
//         for (int i = 0; i < NCHILDPROC; i++)
//             wait();
//     printf(1, "Finished! \n");
//     exit();
// }



#include "types.h"
#include "stat.h"
#include "user.h"

#define NUMPROCS 10
#define MAXNUM 99999999999

int
main(int argc, char *argv[])
{
    int pid;

    for(int i=0; i < NUMPROCS; i++)
    {
        if((pid = fork()) < 0)
        {
            printf(0,"error occured!\n");
            exit();
        }
        if(pid == 0)
            break;
    }

    int k = 1;

    if(pid == 0)
    {
        for (int i = 0; i < MAXNUM; i++)
        {
            for (int j = 0; j < MAXNUM; j++)
            {
                for (int t = 0; t < MAXNUM; t++)
                {
                    k ++;
                }
            }
        }
        exit();
    }

    for (int i = 0; i < NUMPROCS; i++)
        wait();
    exit();
}
