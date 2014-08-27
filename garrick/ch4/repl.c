#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 2048  /* #define because 'magic numbers' suck */

/* Declare a static buffer for user input MAX_SIZE  */

static char input[MAX_SIZE];

int main(int argc, char** argv) {

  /* Print version and exit */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl-C to exit\n");

  /* never ending storrr-y! */
  while(1) {
      /* output our prompt */ 
      fputs("lispy> ", stdout);
      /* get actual input */
      fgets(input, MAX_SIZE, stdin);
      /* echo back to user */
      printf("No you're a %s", input); 
  }

  return 0;
}


