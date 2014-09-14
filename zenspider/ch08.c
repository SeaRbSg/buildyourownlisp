#include "mpc.h"
#include <editline/readline.h>
#include <string.h>
#include <math.h>

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct lval {
  int type;
  union {
    long num;
    int err;
  } v;
} lval;

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM:
    printf("%li", v.v.num);
    break;
  case LVAL_ERR:
    switch (v.v.err) {
    case LERR_DIV_ZERO:
      printf("Error: Division by zero");
      break;
    case LERR_BAD_OP:
      printf("Error: Invalid operator");
      break;
    case LERR_BAD_NUM:
      printf("Error: Invalid number");
      break;
    default:
      printf("Unknown error type: %d", v.v.err);
    }
    break;
  default:
    printf("Unknown lval type: %d", v.type);
  }
}

void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
  if (x.type == LVAL_ERR) return x;
  if (y.type == LVAL_ERR) return y;

  long a = x.v.num;
  long b = y.v.num;

  if (strcmp(op, "+") == 0)   { return lval_num(a + b);          }
  if (strcmp(op, "-") == 0)   { return lval_num(a - b);          }
  if (strcmp(op, "*") == 0)   { return lval_num(a * b);          }
  if (strcmp(op, "/") == 0)   { return b == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(a / b); }
  if (strcmp(op, "%") == 0)   { return b == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(a % b); }
  if (strcmp(op, "^") == 0)   { return lval_num(pow(a, b));      }
  if (strcmp(op, "min") == 0) { return lval_num(a <= b ? a : b); }
  if (strcmp(op, "max") == 0) { return lval_num(a >= b ? a : b); }

  fprintf(stderr, "WARNING: unknown operator '%s'\n", op);
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    long x = atoi(t->contents);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x   = eval(t->children[2]);

  for (int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
    x = eval_op(x, op, eval(t->children[i]));
  }

  return x;
}

long count_leaves(mpc_ast_t* t) {
  long max = t->children_num;

  if (max == 0) {
    return 1;
  } else {
    int total = 0;
    for (int i = 0; i < max; i++) {
      total += count_leaves(t->children[i]);
    }
    return total;
  }
}

int main() {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");
  mpc_result_t r;

  mpca_lang(MPCA_LANG_DEFAULT, "                                \
            number   : /-?[0-9]+(\\.[0-9]+)?/;                  \
            operator : '+' | '-' | '*' | '/' | '%' | '^' | /[a-zA-Z][a-zA-Z0-9-]*/; \
            expr     : <number> | '(' <operator> <expr>+ ')';   \
            lispy    : /^/ <operator> <expr>+ /$/;              ",
            Number, Operator, Expr, Lispy);

  while (1) {
    char * input = readline("lispy> ");
    if (!input) break;
    add_history(input);

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      mpc_ast_print(r.output);

      lval result = eval(r.output);
      printf("= ");
      lval_println(result);
      printf("# %li\n", count_leaves(r.output));

      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);
}
