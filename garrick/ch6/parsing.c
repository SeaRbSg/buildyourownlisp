#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"


int main(int argc, char** argv) {

  /* Create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lithp = mpc_new("lithp");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, 
    "\
    number : /-?[0-9]+/ ; \
    operator : '+' | '-' | '*' | '/' | '%' ; \
    expr : <number> | '(' <operator> <expr>+ ')' ; \
    lithp : /^/ <operator> <expr>+ /$/ ; \
    ",
  Number, Operator, Expr, Lithp);

  /* Print version and exit */
  puts("Lithp Version 0.0.0.0.1");
  puts("Suitable for absolutely nothing!");
  puts("Press Ctrl-C to exit\n");

  /* never ending storrr-y! */
  while(1) {
      /* output our prompt and get input */ 
      char* input = readline("lithp> ");
      /* add to our history */
      add_history(input);
      /* Attempt to parse the user input */
      mpc_result_t r;
      if (mpc_parse("<stdin>", input, Lithp, &r)) {
        /* On success, print the AST */
        mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
      } else {
        /* Otherwise, print the error */
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
      }
      /* free the malocs! */
      free(input);
  }
  /* Undefine and Delete our Parsers.  Or else... */
  mpc_cleanup(4, Number, Operator, Expr, Lithp);
  return 0;
}

/* Bonus work 
 1: ^[ab]*$
 2: ^[[aa*][bb*]]*$
 3: 
 4: 

*/

