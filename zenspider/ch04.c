#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <editline/readline.h>

int main(int argc, char** argv) {
  puts("Lispy version 1.0.0");
  puts("Press ^c to exit\n");

  while (1) {
    char * input = readline("lispy> ");

    if (!input) break;

    add_history(input);

    printf("No you're a %s\n", input);

    free(input);
  }

  return 0;
}
