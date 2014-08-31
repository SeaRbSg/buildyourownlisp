#include <stdio.h>
#include <stdlib.h>


#ifdef _WIN32

#include <string.h>

//declare static buffer "input" for user input
static char input[2048];

//readline for windows: does not allow use of arrow keys
char* readline(char* prompt) {
      fputs("rhl> ", stdout);
      fgets(input, 2048, stdin);
      char* cpy = malloc(strlen(input)+1);
      strcpy(cpy, input);
      cpy[strlen(cpy)-1] = '\0';
      return cpy;
}

//faux add_history for windows
void add_history(char* unused) {}

#else

// to allow for use of arrow keys and prior input access in prompt
#include <editline/readline.h>

#endif

int main(int argc, char** argv){
  
  puts("Rustic Homemade Lisp-thing version 0.0.0.0.1");
  puts("Press Crtl+c to exit\n");

  // input loop
  while (1){
    
    // output prompt text and get input
    char* input = readline("rhl> ");
    
    add_history(input);
    
    printf("yay %s\n", input);

    free(input);
  }
  return 0;
}

