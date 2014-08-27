#include <stdio.h>

#define BUFSIZE 2048
static char input[BUFSIZE];

int main(int argc, char** argv) {
  puts("Lispy version 1.0.0");
  puts("Press ^c to exit\n");

  while (1) {
    printf("lispy> ");
    fgets(input, BUFSIZE, stdin);

    if (feof(stdin)) break;

    printf("No you're a %s\n", input);

  }

  return 0;
}
