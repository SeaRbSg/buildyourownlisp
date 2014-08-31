#include <stdio.h>

//declare static buffer "input" for user input
static char input[2048];

int main(int argc, char** argv){
  
  // welcome in program
  puts("Rustic Homemade Lisp-thing version 0.0.0.0.1");
  puts("Press Crtl+c to exit\n");

  // input loop
  while (1){
    fputs("rhl> ", stdout);
    fgets(input, 2048, stdin);
    printf("yay %s", input);
  }
  return 0;
}

