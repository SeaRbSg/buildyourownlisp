#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>

int main(int argc, char** argv) {

  /* Print version and exit */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl-C to exit\n");

  /* never ending storrr-y! */
  while(1) {
      /* output our prompt and get input */ 
      char* input = readline("lispy> ");
      /* add to our history */
      add_history(input);
      /* echo back to user */
      printf("No you're a %s\n", input); 
      /* free the malocs! */
      free(input);
  }

  return 0;
}


