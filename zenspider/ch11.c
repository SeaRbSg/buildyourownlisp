#include "mpc.h"
#include <editline/readline.h>
#include <string.h>
#include <math.h>

enum { LVAL_NUM, LVAL_SYM, LVAL_SEXP, LVAL_QEXP, LVAL_ERR, LVAL_FUN };

#define LERR_DIV_ZERO       "Division by zero"
#define LERR_BAD_OP         "Invalid operator"
#define LERR_BAD_NUM        "Invalid number"
#define LERR_BAD_SEXP       "S-expression doesn't start with a symbol"
#define LERR_NON_NUMBER     "Cannot operate on non-number"
#define LERR_HEAD_ARITY     "Function 'head' passed too many args"
#define LERR_HEAD_TYPE      "Function 'head' passed incorrect types"
#define LERR_HEAD_EMPTY     "Function 'head' passed empty list"
#define LERR_TAIL_ARITY     "Function 'tail' passed too many args"
#define LERR_TAIL_TYPE      "Function 'tail' passed incorrect types"
#define LERR_TAIL_EMPTY     "Function 'tail' passed empty list"
#define LERR_JOIN_TYPE      "Function 'join' passed incorrect type"
#define LERR_BUILTIN_LOOKUP "Unknown function"
#define LERR_EVAL_ARITY     "Function 'eval' passed too many arguments"
#define LERR_EVAL_TYPE      "Function 'eval' passed incorrect type"
#define LERR_CONS_ARITY     "Function 'cons' passed wrong number of arguments"
#define LERR_CONS_TYPE      "Function 'cons' passed incorrect type on tail"
#define LERR_ENV_GET        "Unbound symbol"

struct lval;
struct lenv;
typedef struct lval*(lbuiltin)(struct lenv*, struct lval*);

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
    lbuiltin* fun;
  } v;
} lval;

typedef struct lenv {
  int count;
  char** syms;
  struct lval** vals;
} lenv;

#define L_COUNT(lval)     (lval)->v.sexp.count
#define L_CELL(lval)      (lval)->v.sexp.cell
#define L_TYPE(lval)      (lval)->type
#define L_FUN(lval)       (lval)->v.fun
#define L_NUM(lval)       (lval)->v.num
#define L_ERR(lval)       (lval)->v.err
#define L_SYM(lval)       (lval)->v.sym

// child accessors
#define L_CELL_N(lval, n) (lval)->v.sexp.cell[(n)]
#define L_TYPE_N(lval, n) L_CELL_N(lval, n)->type
#define L_COUNT_N(lval, n) L_COUNT(L_CELL_N(lval, n))
#define FOREACH_SEXP(i, e) for (int i = 0, _max = L_COUNT(e); i < _max; ++i)

// envs
#define E_COUNT(e) (e)->count
#define E_SYMS(e) (e)->syms
#define E_VALS(e) (e)->vals
#define E_SYM_N(e, i) (e)->syms[i]
#define E_VAL_N(e, i) (e)->vals[i]
#define FOREACH_ENV(i, e) for (int i = 0, _max = E_COUNT(e); i < _max; ++i)

// utilities
#define RETURN_ERR(s, msg) if (1) { lval_del(s); return lval_err(msg); }
#define LOOKUP(a, b) strcmp((a), (b)) == 0
#define SUBSTR(a, b) strstr((a), (b))

#define CHECK_FOR_NUMBERS(a)          \
  FOREACH_SEXP(i, a) {                \
    if (L_TYPE_N(a, i) != LVAL_NUM) { \
      RETURN_ERR(a, LERR_NON_NUMBER); \
    }                                 \
  }

#define FOREACH_NUMBER(a, r, n, code) \
  lval* r = lval_pop(a, 0);           \
                                      \
  while (L_COUNT(a) > 0) {            \
    lval* y = lval_pop(a, 0);         \
    long n = L_NUM(y);                \
                                      \
    code;                             \
                                      \
    lval_del(y);                      \
  }                                   \
                                      \
  lval_del(a);                        \

// prototypes -- via cproto -- I'm not a masochist.

/* ch11.c */
lval *lval_new(void);
lval *lval_err(char *m);
lval *lval_fun(lbuiltin* func);
lval *lval_num(long x);
lval *lval_qexp(void);
lval *lval_sexp(void);
lval *lval_sym(char *s);
lval *lval_copy(lval *v);
void lval_del(lval *v);

lval *lval_add(lval *v, lval *x);
lval *lval_cons(lval *x, lval *s);
lval *lval_join(lval *x, lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);

lenv *lenv_new(void);
void lenv_del(lenv *e);
lval *lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_println(lenv *e);

void lenv_add_builtin(lenv *e, char *name, lbuiltin *func);
void lenv_add_builtins(lenv *e);
lval *lval_eval(lenv *e, lval *v);
lval *lval_eval_sexp(lenv *e, lval *v);
lval *lval_read(mpc_ast_t *t);
lval *lval_read_num(mpc_ast_t *t);
lval *builtin_cons(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval* a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_add(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);
lval *builtin_mod(lenv *e, lval *a);
lval *builtin_exp(lenv *e, lval *a);
lval *builtin_min(lenv *e, lval *a);
lval *builtin_max(lenv *e, lval *a);

long count_leaves(mpc_ast_t *t);
void lval_print(lval *v);
void lval_print_expr(lval *v, char open, char close);
void lval_println(lval *v);

int main(void);

/*
 * Constructors / Destructors
 */

lval* lval_new() {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = -1;
  L_NUM(v) = 0xFFFFFFFF;
  return v;
}

lval* lval_err(char* m) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_ERR;
  L_ERR(v) = strdup(m);
  return v;
}

lval* lval_fun(lbuiltin* func) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_FUN;
  L_FUN(v) = func;
  return v;
}

lval* lval_num(long x) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_NUM;
  L_NUM(v) = x;
  return v;
}

lval* lval_qexp(void) {
  lval* v = lval_sexp();
  L_TYPE(v) = LVAL_QEXP;
  return v;
}

lval* lval_sexp(void) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_SEXP;
  L_COUNT(v) = 0;
  L_CELL(v) = NULL;
  return v;
}

lval* lval_sym(char* s) {
  lval* v = lval_err(s);
  L_TYPE(v) = LVAL_SYM;
  return v;
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  L_TYPE(x) = L_TYPE(v);

  switch (L_TYPE(v)) {
  case LVAL_FUN:
    L_FUN(x) = L_FUN(v);
    break;
  case LVAL_NUM:
    L_NUM(x) = L_NUM(v);
    break;
  case LVAL_ERR:
    L_ERR(x) = strdup(L_ERR(v));
    break;
  case LVAL_SYM:
    L_SYM(x) = strdup(L_ERR(v));
    break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    L_COUNT(x) = L_COUNT(v);
    L_CELL(x)  = malloc(sizeof(lval*) * L_COUNT(x));

    FOREACH_SEXP(i, x) {
      L_CELL_N(x, i) = lval_copy(L_CELL_N(v, i));
    }
    break;
  default:
    printf("Unknown lval type: %d", L_TYPE(v));
    break;
  }

  return x;
}

void lval_del(lval* v) {
  switch (L_TYPE(v)) {
  case LVAL_NUM:
  case LVAL_FUN:
    break;
  case LVAL_SYM:
    free(L_SYM(v));
    break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    FOREACH_SEXP(i, v) {
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

/*
 * Lval Manipulators
 */

lval* lval_add(lval* v, lval *x) {
  L_COUNT(v)++;
  L_CELL(v) = realloc(L_CELL(v), sizeof(lval*) * L_COUNT(v));
  L_CELL_N(v, L_COUNT(v) - 1) = x;
  return v;
}

lval* lval_cons(lval* x, lval *s) {
  size_t size = L_COUNT(s);
  L_COUNT(s)++;
  L_CELL(s) = realloc(L_CELL(s), size+1);
  memmove(&L_CELL_N(s, 1), &L_CELL_N(s, 0), sizeof(lval*) * size);
  L_CELL_N(s, 0) = x;
  return s;
}

lval* lval_join(lval* x, lval* y) {
  while (L_COUNT(y)) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);

  return x;
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

/*
 * Lenv
 */

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  E_COUNT(e) = 0;
  E_SYMS(e) = NULL;
  E_VALS(e) = NULL;
  return e;
}

void lenv_del(lenv* e) {
  FOREACH_ENV(i, e) {
    free(E_SYM_N(e, i));
    lval_del(E_VAL_N(e, i));
  }
  free(E_SYMS(e));
  free(E_VALS(e));
  free(e);
}

lval* lenv_get(lenv* e, lval* k) {
  FOREACH_ENV(i, e) {
    if (LOOKUP(E_SYM_N(e, i), L_SYM(k))) {
      return lval_copy(E_VAL_N(e, i));
    }
  }

  return lval_err(LERR_ENV_GET);
}

void lenv_put(lenv* e, lval* k, lval* v) {
  FOREACH_ENV(i, e) {
    if (LOOKUP(E_SYM_N(e, i), L_SYM(k))) {
      lval_del(E_VAL_N(e, i));
      E_VAL_N(e, i) = lval_copy(v);
      return;
    }
  }

  E_COUNT(e)++;
  E_VALS(e) = realloc(E_VALS(e), sizeof(lval*) * E_COUNT(e));
  E_SYMS(e) = realloc(E_SYMS(e), sizeof(char*) * E_COUNT(e));

  E_VAL_N(e, E_COUNT(e)-1) = lval_copy(v);
  E_SYM_N(e, E_COUNT(e)-1) = strdup(L_SYM(k));
}

void lenv_println(lenv *e) {
  printf("{\n");
  FOREACH_ENV(i, e) {
    printf("  %s = ", E_SYM_N(e, i));
    lval_println(E_VAL_N(e, i));
  }
  printf("}\n");
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin *func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);

  lenv_put(e, k, v);

  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv *e) {
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_exp);

  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "len",  builtin_len);
  lenv_add_builtin(e, "min", builtin_min);
  lenv_add_builtin(e, "max", builtin_max);
}

/*
 * Eval and Read
 */

lval* lval_eval(lenv *e, lval* v) {
  switch (L_TYPE(v)) {
  case LVAL_SEXP:
    return lval_eval_sexp(e, v);
    break;
  case LVAL_SYM: {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
    }
    break;
  default:
    return v; // all other types evaluate to themselves
  }
}

lval* lval_eval_sexp(lenv *e, lval* v) {
  FOREACH_SEXP(i, v) {
    L_CELL_N(v, i) = lval_eval(e, L_CELL_N(v, i));
  }

  FOREACH_SEXP(i, v) {
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

  if (L_TYPE(f) != LVAL_FUN) {
    lval_del(f);
    RETURN_ERR(v, LERR_BAD_SEXP);
  }

  lval* result = L_FUN(f)(e, v);
  lval_del(f);
  return result;
}

lval* lval_read(mpc_ast_t* t) {
  lval* x = NULL;

  if (SUBSTR(t->tag, "number")) { return lval_read_num(t);      }
  if (SUBSTR(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (LOOKUP(t->tag, ">"))      { x = lval_sexp();              }
  if (SUBSTR(t->tag, "sexp"))   { x = lval_sexp();              }
  if (SUBSTR(t->tag, "qexp"))   { x = lval_qexp();              }

  for (int i = 0; i < t->children_num; i++) {
    mpc_ast_t* child = t->children[i];
    char * contents = child->contents;

    if (LOOKUP(contents, "("))        { continue; }
    if (LOOKUP(contents, ")"))        { continue; }
    if (LOOKUP(contents, "{"))        { continue; }
    if (LOOKUP(contents, "}"))        { continue; }
    if (LOOKUP(child->tag,  "regex")) { continue; }

    x = lval_add(x, lval_read(child));
  }

  return x;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
}

/*
 * Builtins
 */

lval* builtin_cons(lenv* e, lval* a) {
  if (L_COUNT(a) != 2)             RETURN_ERR(a, LERR_CONS_ARITY);
  if (L_TYPE_N(a, 1) != LVAL_QEXP) RETURN_ERR(a, LERR_CONS_TYPE);

  lval* x = lval_pop(a, 0);
  lval* s = lval_pop(a, 0);

  return lval_cons(x, s);
}

lval* builtin_eval(lenv *e, lval* a) {
  if (L_COUNT(a) != 1)             RETURN_ERR(a, LERR_EVAL_ARITY);
  if (L_TYPE_N(a, 0) != LVAL_QEXP) RETURN_ERR(a, LERR_EVAL_TYPE);

  lval* x = lval_take(a, 0);
  L_TYPE(x) = LVAL_SEXP; // TODO: ARGH

  return lval_eval(e, x);
}

lval* builtin_head(lenv* e, lval* a) {
  if (L_COUNT(a) != 1)              RETURN_ERR(a, LERR_HEAD_ARITY);
  if (L_TYPE_N(a, 0) != LVAL_QEXP)  RETURN_ERR(a, LERR_HEAD_TYPE);
  if (L_COUNT(L_CELL_N(a, 0)) == 0) RETURN_ERR(a, LERR_HEAD_EMPTY);

  lval* v = lval_take(a, 0);

  while(L_COUNT(v) > 1) { // WTF? this seems REALLY bad!
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_join(lenv* e, lval* a) {
  FOREACH_SEXP(i, a) {
    if (L_TYPE_N(a, i) != LVAL_QEXP) RETURN_ERR(a, LERR_JOIN_TYPE);
  }

  lval* x = lval_pop(a, 0);

  while (L_COUNT(a)) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);

  return x;
}

lval* builtin_len(lenv* e, lval* a) {
  return lval_num(L_COUNT_N(a, 0));
}

lval* builtin_list(lenv* e, lval* a) {
  L_TYPE(a) = LVAL_QEXP; // TODO: these are all mutating lval. HORRIBLE.

  return a;
}

lval* builtin_tail(lenv* e, lval* a) {
  if (L_COUNT(a) != 1)              RETURN_ERR(a, LERR_TAIL_ARITY);
  if (L_TYPE_N(a, 0) != LVAL_QEXP)  RETURN_ERR(a, LERR_TAIL_TYPE);
  if (L_COUNT(L_CELL_N(a, 0)) == 0) RETURN_ERR(a, LERR_TAIL_EMPTY);

  lval* v = lval_take(a, 0);

  lval_del(lval_pop(v, 0)); // WTF? This seems really bad!

  return v;
}

lval *builtin_add(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) += n);
  return result;
}

lval *builtin_sub(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);

  lval* x = lval_pop(a, 0);

  // FOREACH_NUMBER(a, result, n, L_NUM(result) += n);
  // return result;

  if (L_COUNT(a) == 0) {
    L_NUM(x) *= -1;
  }

  while (L_COUNT(a) > 0) {
    lval* y = lval_pop(a, 0);

    long b = L_NUM(y);

    L_NUM(x) -= b;

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval *builtin_mul(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) *= n);
  return result;
}

lval *builtin_div(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) /= n);
  return result;
}

lval *builtin_mod(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) %= n);
  return result;
}

lval *builtin_exp(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) ^= n);
  return result;
}

lval *builtin_min(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, long m = L_NUM(result); L_NUM(result) = n <= m ? n : m);
  return result;
}

lval *builtin_max(lenv *e, lval *a) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, long m = L_NUM(result); L_NUM(result) = n >= m ? n : m);
  return result;
}

/*
 * Main & Side effect free
 */

long count_leaves(mpc_ast_t* t) { // TODO remove me or hook me in
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

void lval_print(lval* v) {
  switch (L_TYPE(v)) {
  case LVAL_NUM:
    printf("%li", L_NUM(v));
    break;
  case LVAL_FUN:
    printf("<function>");
    break;
  case LVAL_SYM:
    printf("%s", L_SYM(v));
    break;
  case LVAL_SEXP:
    lval_print_expr(v, '(', ')');
    break;
  case LVAL_QEXP:
    lval_print_expr(v, '{', '}');
    break;
  case LVAL_ERR:
    printf("Error: %s", L_ERR(v));
    break;
  default:
    printf("Unknown lval type: %d", L_TYPE(v));
  }
}

void lval_print_expr(lval* v, char open, char close) {
  putchar(open);

  FOREACH_SEXP(i, v) {
    lval_print(L_CELL_N(v, i));

    if (i != _max) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
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
            symbol   : /[a-zA-Z0-9_+*\\/\\\\=<>!&-]+/;           \
            sexp     : '(' <expr>* ')';                         \
            qexp     : '{' <expr>* '}';                         \
            expr     : <number> | <symbol> | <sexp> | <qexp>;   \
            lispy    : /^/ <expr>* /$/;                         ",
            Number, Symbol, Sexp, Qexp, Expr, Lispy);

  lenv* env = lenv_new();
  lenv_add_builtins(env);

  while (1) {
    char * input = readline("lispy> ");
    if (!input) break;
    add_history(input);

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval* x = lval_eval(env, lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  free(env);
  mpc_cleanup(6, Number, Symbol, Sexp, Qexp, Expr, Lispy);

  return 0;
}
