#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>


/* Declare New lval Struct */
typedef struct { 
  int type;
  long num;
  int err;
} lval;

/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };

/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create a new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* Print an "lval" */
void lval_print(lval v) {
  switch (v.type) {
  /* If the type is a number print it, then 'break' out of the switch. */
  case LVAL_NUM: 
    printf("%li", v.num);
    break;
  /* In the case the type is an error */
  case LVAL_ERR:
    /* Check what exact type of error it is and print it */
    if (LERR_DIV_ZERO == v.err) { 
      printf("Error: Division By Zero!"); 
    }
    if (LERR_BAD_OP == v.err) { 
      printf("Error: Invalid Operator!"); 
    }
    if (LERR_BAD_NUM == v.err) { 
      printf("Error: Invalid Number!"); 
    }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

  if (LVAL_ERR == x.type) { 
    return x; 
  }
  if (LVAL_ERR == y.type) { 
    return y;
  }

  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
  if (strcmp(op, "%") == 0) { 
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num); 
  }
  if (strcmp(op, "/") == 0) { 
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  if (strcmp(op, "min") == 0) { 
    return x.num <= y.num ? lval_num(x.num) : lval_num(y.num); 
  }
  if (strcmp(op, "max") == 0) { 
    return x.num >= y.num ? lval_num(x.num) : lval_num(y.num); 
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) { 
    /* Check if there is some error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  if (t->children_num == 4 && strcmp(op, "-") == 0) {
    x = lval_num(x.num * -1);
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

      lval result = eval(r.output);
      lval_println(result);
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
