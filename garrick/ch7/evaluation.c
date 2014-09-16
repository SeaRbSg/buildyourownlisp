#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"

long our_exponent(long base, long exp) {
  if(exp <= 1) {
    return base;
    }
  else {
    return base * our_exponent(base, exp - 1);
  }
}

long min(long x, long y) {
  if(x <= y) {
    return x;
    } 
  return y;
}

long max(long x, long y) {
  if(x >= y) {
    return x;
    } 
  return y;
}

/* Use operator string to see which operator to perform */
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "^") == 0) { return our_exponent( x, y); }
  if (strcmp(op, "min") == 0) { return min( x, y); }
  if (strcmp(op, "max") == 0) { return max( x, y); }
  return 0;
}

/* Let us eval */
long eval(mpc_ast_t* t) {

  /* If a number, return directly otherwise expression */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* operator is always 2nd child */
  char* op = t->children[1]->contents;

  /* store the 3rd child in x */
  int x = eval(t->children[2]);

  /* Iterate the remaining children, combining using our operator */
  int i = 3;
  while(strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}


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
    operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
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
        //mpc_ast_print(r.output);
        /* Get a real result and print it */
        long result = eval(r.output);
        printf("%li\n", result);
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


