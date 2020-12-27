#include "types.h"
#include "stat.h"
#include "user.h"
#include "usr_lock.h"
#define TRUE 1
#define BUFF_SIZE 512

int read_count = 0;

char buf[BUFF_SIZE];

void
cat(int fd)
{
  int n;

  while((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      printf(1, "cat: write error\n");
      exit();
    }
  }
  if(n < 0){
    printf(1, "cat: read error\n");
    exit();
  }
}

void reader(struct condvar* writeCondition, struct condvar* readerCondition){
    while (amu_get(writeCondition) == 1) ;
    amu_inc(readerCondition);
    int fd=open("buffer.txt", 0);
    printf(2, "Read from buffer: ");
    cat(fd);
    close(fd);
    amu_dec(readerCondition);
    if(amu_get(readerCondition) == 0){
        if (amu_get(writeCondition) == 1)
            cv_signal(writeCondition);
    }
}

void writer(struct condvar* writeCondition, struct condvar* readerCondition){
    if (amu_get(readerCondition) > 0){
        amu_wait(writeCondition);
    }
    amu_inc(writeCondition);
    char c = 'a';
    for(int i = 0; i < 5 - 1; i++){
    if(c + i > 'z'){
        buf[i] = (c + i) - ('z' - 'a' + 1);
    }
    else{
        buf[i] = c + i;
    }
    }
    buf[5 - 1] = '\0';
    int fd = open("buffer.txt", 0x001 | 0x200); 
    write(fd, buf,sizeof(buf) + 3);
    write(fd,"\n",2);
    close(fd);
    printf(1, "wrote to file: %s\n", buf);

    amu_dec(writeCondition);
}

int main(){
    
    int children = 10;
    struct condvar* writerCondition;
    struct condvar* readerCondition;
    readerCondition = cv_init();
    writerCondition = cv_init();
    int pid = fork();
    if (pid == 0){
        for (int j = 0 ; j < children; j++){
            int pid2 = fork();
            if (pid2 == 0){
                reader(writerCondition, readerCondition);
                exit();
            }
        }
    }else{
        writer(writerCondition,readerCondition);
    }
    for (int i = 0; i < children ; i++)
        wait();
    exit();
}