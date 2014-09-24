#include "mpc.h"
#include <editline/readline.h>

static char input[2048];

int main() {
  /* Create Some Parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, 
            "                                                      \
            number :   /-?[0-9]+(\\.[0-9]+)?/ ;                                \
            operator:   '+' | '-' | '*' | '/' | '%' | /add/ | /sub/ | /mul/ | /div/ ;      \
            expr : <number> | '(' <operator> <expr>+ ')' ;         \
            lispy : /^/ <expr> (<operator> <expr>)+ /$/ ;                   \
            ",
            Number, Operator, Expr, Lispy);

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Crtl-c to Exit\n");

  /* In a never ending loop */
  while (1) {
    
    /* Output our prompt */
    fputs("lispy> ", stdout);

    /* Read a line of user input of maximum size 2048 */
    fgets(input, 2048, stdin);

    /* Attempt to Parse the user Input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On Success Print the AST */
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
