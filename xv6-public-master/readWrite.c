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
    while(TRUE){
        cv_wait(readerCondition);
        read_count++;
        if(read_count == 1){
            cv_wait(writeCondition);
        }
        cv_signal(readerCondition);

        int fd=open("buffer.txt", 0);
        printf(2, "Read from buffer: ");
        cat(fd);
        close(fd);

        cv_wait(readerCondition);
        read_count--;
        if(read_count == 0)
            cv_signal(writeCondition);
        cv_signal(readerCondition);
    }
}
void writer(struct condvar* writeCondition){
    char c;
    for(c='a'; c <= 'z'; c++){
        cv_wait(writeCondition);
        
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

        cv_signal(writeCondition);
    }
}

int main(){
    struct condvar* writerCondition = (struct condvar*)malloc(sizeof(struct condvar));
    struct condvar* readerCondition = (struct condvar*)malloc(sizeof(struct condvar));
    cv_init(readerCondition);
    init_lock(&readerCondition->lock);
    cv_init(writerCondition);
    init_lock(&writerCondition->lock);
    int pid = fork();
    if (pid == 0){
        reader();
        exit();
    }else{
        consumer();
    }
    wait();
    exit();
}