#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  //printf("%d %d\n", f(8)+1, 13);
  unsigned int i = 0x00646c72;
  int c1=0x64;
  int c2=0x6c;
  int c3=0x72;
  printf("%c%c%c\n",c1,c2,c3);
	printf("H%x Wo%s\n", 57616, &i);
  printf("x=%d y=%d", 3);
  exit(0);
}
