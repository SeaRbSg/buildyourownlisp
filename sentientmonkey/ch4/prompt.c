#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

int main(int argc, char** argv) {

    /* follow semvar */
    puts("Lispy Version 0.0.1");
    /* Ctrl-D for exit */
    puts("Press Ctrl+d to Exit\n");

    while(1) {
        char* input = readline("lispy> ");
        add_history(input);

        // break if we don't have input
        if (!input) {  break; }

        printf("No, you're a %s\n", input);

        free(input);
    }
    return 0;
}
