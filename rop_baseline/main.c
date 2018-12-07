#include <stdio.h>

int add(int a, int b);

int main(int argc, char **argv)
{
  printf("Hello, starting ROP...\n");
  //printf("%d\n", add(2, 6));
  add(2,6);
  exit(0);

  return 0;
}
