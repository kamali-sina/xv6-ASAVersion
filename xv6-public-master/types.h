typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
struct run *get_freelist();
struct run {
  struct run *next;
};
void _inc_ref(int fd);
void inc_ref(int fd);
void print_ref(int fd);