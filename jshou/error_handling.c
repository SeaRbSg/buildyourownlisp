#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"

typedef struct {
  int type;
  long num;
  int err;
} lval;

enum { LVAL_NUM, LVAL_ERR }; // lval_types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM }; // error codes

lval lval_num(long x) {
  lval lv;
  lv.type = LVAL_NUM;
  lv.num = x;

  return lv;
}

lval lval_err(int err) {
  lval lv;
  lv.type = LVAL_ERR;
  lv.err = err;

  return lv;
}

void lval_print(lval v) {
  switch(v.type) {
    case LVAL_NUM:
      printf("%li", v.num);
    break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) { printf("Error: Division by zero"); }
      if (v.err == LERR_BAD_OP) { printf("Error: Invalid operator"); }
      if (v.err == LERR_BAD_NUM) { printf("Error: Invalid number"); }
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
  // if there's an error with the numbers
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    if (y.num == 0) {
      return lval_err(LERR_DIV_ZERO);
    } else {
      return lval_num(x.num / y.num);
    }
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* JoshLisp = mpc_new("joshlisp");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      joshlisp    : /^/ <operator> <expr>+ /$/ ;             \
      ", Number, Operator, Expr, JoshLisp);

  printf("JoshLisp Version 0.0.0.0.3\n");

  while (1) {
    mpc_result_t r;
    char* input = readline("josh_lisp> ");
    add_history(input);

    if (!input)
      break;

    if (mpc_parse("<stdin>", input, JoshLisp, &r)) {
      mpc_ast_t* ast = r.output;
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(ast);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, JoshLisp);
  return 0;
}
