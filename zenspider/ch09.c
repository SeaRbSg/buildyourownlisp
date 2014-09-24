#include "mpc.h"
#include <editline/readline.h>
#include <string.h>
#include <math.h>

enum { LVAL_NUM, LVAL_SYM, LVAL_SEXP, LVAL_ERR };

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

// prototypes -- via cproto -- I'm not a masochist.

/* ch09.c */
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexp(void);
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
  v->type = LVAL_NUM;
  v->v.num = x;
  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->v.err = malloc(strlen(m) + 1);
  strcpy(v->v.err, m);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->v.sym = malloc(strlen(s) + 1);
  strcpy(v->v.sym, s);
  return v;
}

lval* lval_sexp(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXP;
  v->v.sexp.count = 0;
  v->v.sexp.cell = NULL;
  return v;
}

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->v.num);
    break;
  case LVAL_SYM:
    printf("%s", v->v.sym);
    break;
  case LVAL_SEXP:
    lval_expr_print(v, '(', ')');
    break;
  case LVAL_ERR:
    printf("Error: %s", v->v.err);
    break;
  default:
    printf("Unknown lval type: %d", v->type);
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXP) {
    return lval_eval_sexp(v);
  }

  // all other types evaluate to themselves
  return v;
}

lval* lval_add(lval* v, lval *x) {
  v->v.sexp.count++;
  v->v.sexp.cell = realloc(v->v.sexp.cell, sizeof(lval*) * v->v.sexp.count);
  v->v.sexp.cell[v->v.sexp.count - 1] = x;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
  case LVAL_NUM:
    break;
  case LVAL_SYM:
    free(v->v.sym);
    break;
  case LVAL_SEXP:
    for (int i = 0; i < v->v.sexp.count; i++) {
      lval_del(v->v.sexp.cell[i]);
    }
    free(v->v.sexp.cell);
    break;
  case LVAL_ERR:
    free(v->v.err);
    break;
  default:
    printf("Unknown lval type: %d", v->type);
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

  int max = v->v.sexp.count - 1;

  for (int i = 0; i < v->v.sexp.count; i++) {
    lval_print(v->v.sexp.cell[i]);

    if (i != max) {
      putchar(' ');
    }
  }

  putchar(close);
}

lval* lval_pop(lval* v, int i) {
  sexp * s = &v->v.sexp;
  lval* x = v->v.sexp.cell[i];

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
  for (int i = 0; i < a->v.sexp.count; i++) {
    if (a->v.sexp.cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err(LERR_NON_NUMBER);
    }
  }

  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->v.sexp.count == 0) {
    x->v.num *= -1;
  }

  while (a->v.sexp.count > 0) {
    lval* y = lval_pop(a, 0);

    long a = x->v.num;
    long b = y->v.num;

    // TODO: convert to lookup table
    if (strcmp(op, "+") == 0)   { x->v.num += b; }
    if (strcmp(op, "-") == 0)   { x->v.num -= b; }
    if (strcmp(op, "*") == 0)   { x->v.num *= b; }
    if (strcmp(op, "/") == 0)   {
      if (b == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err(LERR_DIV_ZERO);
        break;
      }
      x->v.num /= b;
    }
    if (strcmp(op, "%") == 0) {
      if (b == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err(LERR_DIV_ZERO);
        break;
      }
      x->v.num %= b;
    }
    if (strcmp(op, "^") == 0)   {
      x->v.num = pow(a, b);
    }
    if (strcmp(op, "min") == 0) { x->v.num = a <= b ? a : b; }
    if (strcmp(op, "max") == 0) { x->v.num = a >= b ? a : b; }

    // TODO?
    // fprintf(stderr, "WARNING: unknown operator '%s'\n", op);
    // return lval_err(LERR_BAD_OP);

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* lval_eval_sexp(lval* v) {
  for (int i = 0; i < v->v.sexp.count; i++) {
    v->v.sexp.cell[i] = lval_eval(v->v.sexp.cell[i]);
  }

  for (int i = 0; i < v->v.sexp.count; i++) {
    if (v->v.sexp.cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->v.sexp.count == 0) {
    return v;
  }

  if (v->v.sexp.count == 1) {
    return lval_take(v, 0);
  }

  lval* f = lval_pop(v, 0);

  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err(LERR_BAD_SEXP);
  }

  lval* result = builtin_op(v, f->v.sym);
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
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");
  mpc_result_t r;

  mpca_lang(MPCA_LANG_DEFAULT, "                                \
            number   : /-?[0-9]+(\\.[0-9]+)?/;                  \
            symbol   : '+' | '-' | '*' | '/' | '%' | '^' | /[a-zA-Z][a-zA-Z0-9-]*/; \
            sexp     : '(' <expr>* ')';                         \
            expr     : <number> | <symbol> | <sexp>;            \
            lispy    : /^/ <expr>* /$/;                         ",
            Number, Symbol, Sexp, Expr, Lispy);

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

  mpc_cleanup(5, Number, Symbol, Sexp, Expr, Lispy);
}
