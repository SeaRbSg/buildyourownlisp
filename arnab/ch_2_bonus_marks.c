#include <stdio.h>

void say_hello(int n){
  int i;
  for (i = 0; i < n; i++) {
    puts("Hello World!");
  }
}

int main(int argc, char *argv[])
{
  puts("##### for loop #####");
  int i;
  for (i = 0; i < 5; i++) {
    puts("Hello World!");
  }

  puts("\n##### while loop #####");
  i = 0;
  while (i < 5) {
    puts("Hello World!");
    i++;
  }

  puts("\n##### using a fn ######");
  say_hello(5);

  return 0;
}
