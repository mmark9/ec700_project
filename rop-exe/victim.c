#include <stdio.h>
void not_used()
{
  system("/bin/sh");
}
int main() {
  char name[64];
  puts("What's your name?");
  gets(name);
  printf("Hello, %s!\n", name);
  return 0;
}