#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "^") == 0) { return pow(x, y); }
  if (strcmp(op, "min") == 0) { return x <= y ? x : y; }
  if (strcmp(op, "max") == 0) { return x >= y ? x : y; }

  return 0;
}

long eval(mpc_ast_t* t) {

  /* If tagged as number return it directly, otherwise expression. */
  if (strstr(t->tag, "number")) { 
    return atoi(t->contents); 
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x` */
  long x = eval(t->children[2]);

  if (t->children_num == 4 && strcmp(op, "-") == 0) {
    x = x * -1;
  } else {
    /* Iterate the remaining children, combining using our operator */
    for (int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
      x = eval_op(x, op, eval(t->children[i]));
    }
  }
  
  return x;
}

int main() {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT, 
            "                                                      \
            number :   /-?[0-9]+(\\.[0-9]+)?/ ;                                \
            operator:   '+' | '-' | '*' | '/' | '%' | '^' | /min/ | /max/ | /add/ | /sub/ | /mul/ | /div/ ;      \
            expr : <number> | '(' <operator> <expr>+ ')' ;         \
            lispy    : /^/ <operator> <expr>+ /$/ ;                \
            ",
            Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Press Crtl-c to Exit\n");

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      mpc_ast_print(r.output);

      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);

    } else {
      /* Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
