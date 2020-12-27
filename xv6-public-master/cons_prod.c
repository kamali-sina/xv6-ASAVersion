#include "types.h"
#include "stat.h"
#include "user.h"
#define EMPTY 0
#define MUTEX 1
#define FULL 2
#define BUFFER_SIZE 6

char buffer[BUFFER_SIZE];
char buf[512];

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

void producer(void){
  char c;
  for(c='a'; c <= 'z'; c++){
    // produce
    // c it self
    semaphore_aquire(EMPTY);
    semaphore_aquire(MUTEX);

    for(int i = 0; i < BUFFER_SIZE - 1; i++){
      if(c + i > 'z'){
        buffer[i] = (c + i) - ('z' - 'a' + 1);
      }
      else{
        buffer[i] = c + i;
      }
    }
    buffer[BUFFER_SIZE - 1] = '\0';
    int fd = open("buffer.txt", 0x001 | 0x200); 
    write(fd, buffer,sizeof(buffer) + 3);
    write(fd,"\n",2);
    close(fd);

    printf(2, "Wrote %s to buffer\n", buffer);
    semaphore_release(MUTEX);
    semaphore_release(FULL);
  }
}
void consumer(void){
  char c;
  for(c='a'; c <= 'z'; c++){
    semaphore_aquire(FULL);
    semaphore_aquire(MUTEX);
    // printf(2, "Read %s from buffer\n", buffer);
    // printf(2, "Read %s from buffer\n", buffer);
    int fd=open("buffer.txt", 0);
    printf(2, "Read from buffer: ");
    cat(fd);
    close(fd);

    semaphore_release(MUTEX);
    semaphore_release(EMPTY);
    
    //consume
  }
}


int
main(int argc, char *argv[])
{
  semaphore_initialize(EMPTY,1,0);
  semaphore_initialize(FULL,0,0);
  semaphore_initialize(MUTEX,1,0);
  int pid = fork();
  if (pid == 0){
    producer();
    exit();
  }else{
    consumer();
  }
  wait();
  exit();
}