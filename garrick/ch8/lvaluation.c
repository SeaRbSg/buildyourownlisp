#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"

/* enumeration of possible eval types */
enum { LVAL_NUM, LVAL_ERR };

/* enumeration of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* type, type baby */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* new error type lval */
lval lval_err(long err) {
  lval v;
  v.type = LVAL_ERR;
  v.err = err;
  return v;
}

/* Print an lval */
void lval_print(lval v) {
  switch (v.type) {
    /* print & break on a number */
    case LVAL_NUM: {
      printf("%li", v.num);
      break;
    }
    /* print & break for some error */
    case LVAL_ERR: {
      /* print exact error message */
      if (v.err == LERR_DIV_ZERO) { printf("Error: Divide by zero!"); }
      if (v.err == LERR_BAD_OP)   { printf("Error: Invalid operator!"); }
      if (v.err == LERR_BAD_NUM)  { printf("Error: Invalid number!"); }
      break;
    }
  }
}

/* print and lval followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval our_exponent(lval base, lval exp) {
  if(exp.num <= 1) {
    return base;
    }
  else {
    return lval_num(base.num * our_exponent(base, lval_num(exp.num - 1)).num);
  }
}

lval min(lval x, lval y) {
  if(x.num <= y.num) {
    return x;
    } 
  return y;
}

lval max(lval x, lval y) {
  if(x.num >= y.num) {
    return x;
    } 
  return y;
}

/* Use operator string to see which operator to perform */
lval eval_op(lval x, char* op, lval y) {
  /* bail out on errors*/
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* math up the numbers */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { 
      return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); 
    }
  if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
  if (strcmp(op, "^") == 0) { return our_exponent( x, y); }
  if (strcmp(op, "min") == 0) { return min( x, y); }
  if (strcmp(op, "max") == 0) { return max( x, y); }

  /* wtf lol idk */
  return lval_err(LERR_BAD_OP);
}

/* Let us eval */
lval eval(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) {
    /* check for an error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM); 
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

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
        lval result = eval(r.output);
        lval_println(result);
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


