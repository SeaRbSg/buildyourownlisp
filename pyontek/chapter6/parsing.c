#include "mpc.h"
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
 
  // Polish notation parser

  // create parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define parsers
  mpca_lang(MPCA_LANG_DEFAULT,
     "                                                   \
     number   : /-?[0-9]+/ ;                             \
     operator : '+' | '-' | '*' | '/' ;                  \
     expr     : <number> | '(' <operator> <expr>+ ')' ;  \
     lispy    : /^/ <operator> <expr>+ /$/ ;             \
     ",
     Number, Operator, Expr, Lispy);


  // Begin prompt
  puts("Rustic Homemade Lisp-thing version 0.0.0.0.1");
  puts("Press Crtl+c to exit\n");

  // input loop
  while (1){
    
    // output prompt text and get input
    char* input = readline("rhl> ");
    
    add_history(input);
    
    // parse that shit (user input)
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){
        // success!
        mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
    }
    else {
        // failure: print error
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
    
    free(input);
  }

  // Undefine and Delete Parsers
  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}



