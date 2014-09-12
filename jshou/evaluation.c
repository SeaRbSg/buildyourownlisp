#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"

long eval(mpc_ast_t* t);
long eval_op(long x, char* op, long y);
int num_leaves(mpc_ast_t* t);

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
      long result = eval(r.output);
      printf("%li\n", result);
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

long eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return atoi(t->contents); }

  char* op = t->children[1]->contents;
  long x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

int num_leaves(mpc_ast_t* t) {
  if (t->children_num < 1) {
    // no children
    return 1;
  } else {
    // iterate through children and return the sum
    int sum = 0;

    for (int i = 0; i < t->children_num; i++) {
      sum += num_leaves(t->children[i]);
    }

    return sum;
  }
}
