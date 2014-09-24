#include "mpc.h"
#include <editline/readline.h>
#include <string.h>
#include <math.h>

enum { LVAL_NUM, LVAL_SYM, LVAL_SEXP, LVAL_QEXP, LVAL_ERR };

#define LERR_DIV_ZERO "Division by zero"
#define LERR_BAD_OP   "Invalid operator"
#define LERR_BAD_NUM  "Invalid number"
#define LERR_BAD_SEXP "S-expression doesn't start with a symbol"
#define LERR_NON_NUMBER "Cannot operate on non-number"

typedef struct sexp {
  int count;
  struct lval** cell;
} sexp;

typedef struct lval {
  int type;
  union {
    long num;
    char* err;
    char* sym;
    sexp sexp;
  } v;
} lval;

#define L_COUNT(lval) (lval)->v.sexp.count
#define L_CELL(lval)  (lval)->v.sexp.cell
#define L_CELL_N(lval, n)  (lval)->v.sexp.cell[(n)]
#define L_TYPE(lval)  (lval)->type
#define L_TYPE_N(lval, n) L_CELL_N(lval, n)->type
#define L_NUM(lval)   (lval)->v.num
#define L_ERR(lval)   (lval)->v.err
#define L_SYM(lval)   (lval)->v.sym

// prototypes -- via cproto -- I'm not a masochist.

/* ch09.c */
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexp(void);
lval *lval_qexp(void);
lval *lval_add(lval *v, lval *x);
void lval_del(lval *v);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *lval_eval_sexp(lval *v);
lval *lval_eval(lval *v);
int main(void);

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = LVAL_NUM;
  L_NUM(v) = x;
  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = LVAL_ERR;
  L_ERR(v) = malloc(strlen(m) + 1);
  strcpy(L_ERR(v), m);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = LVAL_SYM;
  L_SYM(v) = malloc(strlen(s) + 1);
  strcpy(L_SYM(v), s);
  return v;
}

lval* lval_sexp(void) {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = LVAL_SEXP;
  L_COUNT(v) = 0;
  L_CELL(v) = NULL;
  return v;
}

lval* lval_qexp(void) {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = LVAL_QEXP;
  L_COUNT(v) = 0;
  L_CELL(v) = NULL;
  return v;
}

void lval_print(lval* v) {
  switch (L_TYPE(v)) {
  case LVAL_NUM:
    printf("%li", L_NUM(v));
    break;
  case LVAL_SYM:
    printf("%s", L_SYM(v));
    break;
  case LVAL_SEXP:
    lval_expr_print(v, '(', ')');
    break;
  case LVAL_QEXP:
    lval_expr_print(v, '{', '}');
    break;
  case LVAL_ERR:
    printf("Error: %s", L_ERR(v));
    break;
  default:
    printf("Unknown lval type: %d", L_TYPE(v));
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_eval(lval* v) {
  if (L_TYPE(v) == LVAL_SEXP) {
    return lval_eval_sexp(v);
  }

  // all other types evaluate to themselves
  return v;
}

lval* lval_add(lval* v, lval *x) {
  L_COUNT(v)++;
  L_CELL(v) = realloc(L_CELL(v), sizeof(lval*) * L_COUNT(v));
  L_CELL_N(v, L_COUNT(v) - 1) = x;
  return v;
}

void lval_del(lval* v) {
  switch (L_TYPE(v)) {
  case LVAL_NUM:
    break;
  case LVAL_SYM:
    free(L_SYM(v));
    break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    for (int i = 0; i < L_COUNT(v); i++) {
      lval_del(L_CELL_N(v, i));
    }
    free(L_CELL(v));
    break;
  case LVAL_ERR:
    free(L_ERR(v));
    break;
  default:
    printf("Unknown lval type: %d", L_TYPE(v));
    break;
  }
  free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
}

lval* lval_read(mpc_ast_t* t) {
  lval* x = NULL;

  if (strstr(t->tag, "number")) { return lval_read_num(t);      }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (strcmp(t->tag, ">") == 0) { x = lval_sexp();              }
  if (strstr(t->tag, "sexp"))   { x = lval_sexp();              }
  if (strstr(t->tag, "qexp"))   { x = lval_qexp();              }

  for (int i = 0; i < t->children_num; i++) {
    mpc_ast_t* child = t->children[i];
    char * contents = child->contents;

    if (strcmp(contents, "(") == 0)        { continue; }
    if (strcmp(contents, ")") == 0)        { continue; }
    if (strcmp(contents, "{") == 0)        { continue; }
    if (strcmp(contents, "}") == 0)        { continue; }
    if (strcmp(child->tag,  "regex") == 0) { continue; }

    x = lval_add(x, lval_read(child));
  }

  return x;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);

  int max = L_COUNT(v) - 1;

  for (int i = 0; i < L_COUNT(v); i++) {
    lval_print(L_CELL_N(v, i));

    if (i != max) {
      putchar(' ');
    }
  }

  putchar(close);
}

lval* lval_pop(lval* v, int i) {
  sexp * s = &v->v.sexp;
  lval* x = L_CELL_N(v, i);

  memmove(&s->cell[i], &s->cell[i+1], sizeof(lval*) * (s->count-i-1));
  s->count--;

  s->cell = realloc(s->cell, sizeof(lval*) * s->count);

  return x;
}

lval* lval_take(lval* v, int i) { // TODO: prove lval_del is appropriate
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {
  for (int i = 0; i < L_COUNT(a); i++) {
    if (L_TYPE_N(a, i) != LVAL_NUM) {
      lval_del(a);
      return lval_err(LERR_NON_NUMBER);
    }
  }

  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && L_COUNT(a) == 0) {
    L_NUM(x) *= -1;
  }

  while (L_COUNT(a) > 0) {
    lval* y = lval_pop(a, 0);

    long a = L_NUM(x);
    long b = L_NUM(y);

    // TODO: convert to lookup table
    if (strcmp(op, "+") == 0)   { L_NUM(x) += b; }
    if (strcmp(op, "-") == 0)   { L_NUM(x) -= b; }
    if (strcmp(op, "*") == 0)   { L_NUM(x) *= b; }
    if (strcmp(op, "/") == 0)   {
      if (b == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err(LERR_DIV_ZERO);
        break;
      }
      L_NUM(x) /= b;
    }
    if (strcmp(op, "%") == 0) {
      if (b == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err(LERR_DIV_ZERO);
        break;
      }
      L_NUM(x) %= b;
    }
    if (strcmp(op, "^") == 0)   {
      L_NUM(x) = pow(a, b);
    }
    if (strcmp(op, "min") == 0) { L_NUM(x) = a <= b ? a : b; }
    if (strcmp(op, "max") == 0) { L_NUM(x) = a >= b ? a : b; }

    // TODO?
    // fprintf(stderr, "WARNING: unknown operator '%s'\n", op);
    // return lval_err(LERR_BAD_OP);

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* lval_eval_sexp(lval* v) {
  for (int i = 0; i < L_COUNT(v); i++) {
    L_CELL_N(v, i) = lval_eval(L_CELL_N(v, i));
  }

  for (int i = 0; i < L_COUNT(v); i++) {
    if (L_TYPE_N(v, i) == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (L_COUNT(v) == 0) {
    return v;
  }

  if (L_COUNT(v) == 1) {
    return lval_take(v, 0);
  }

  lval* f = lval_pop(v, 0);

  if (L_TYPE(f) != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err(LERR_BAD_SEXP);
  }

  lval* result = builtin_op(v, L_SYM(f));
  lval_del(f);
  return result;
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
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexp     = mpc_new("sexp");
  mpc_parser_t* Qexp     = mpc_new("qexp");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");
  mpc_result_t r;

  mpca_lang(MPCA_LANG_DEFAULT, "                                \
            number   : /-?[0-9]+(\\.[0-9]+)?/;                  \
            symbol   : '+' | '-' | '*' | '/' | '%' | '^' | /[a-zA-Z][a-zA-Z0-9-]*/; \
            sexp     : '(' <expr>* ')';                         \
            qexp     : '{' <expr>* '}';                         \
            expr     : <number> | <symbol> | <sexp> | <qexp>;   \
            lispy    : /^/ <expr>* /$/;                         ",
            Number, Symbol, Sexp, Qexp, Expr, Lispy);

  while (1) {
    char * input = readline("lispy> ");
    if (!input) break;
    add_history(input);

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(6, Number, Symbol, Sexp, Qexp, Expr, Lispy);
}
