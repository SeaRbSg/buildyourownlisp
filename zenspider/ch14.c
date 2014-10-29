#include "mpc.h"
#include <editline/readline.h>
#include <string.h>
#include <math.h>

enum { LVAL_NUM,   // 0
       LVAL_SYM,   // 1
       LVAL_STR,   // 2
       LVAL_SEXP,  // 3
       LVAL_QEXP,  // 4
       LVAL_ERR,   // 5
       LVAL_FUN,   // 6
       LVAL_LAM }; // 7

#define LERR_ARITY "Function '%s' passed wrong number of arguments. %d != %d."
#define LERR_BAD_NUM        "Invalid number"
#define LERR_BAD_SEXP       "S-expression doesn't start with a symbol"
#define LERR_NON_NUMBER     "Cannot operate on non-number"
#define LERR_HEAD_EMPTY     "Function 'head' passed empty list"
#define LERR_TAIL_EMPTY     "Function 'tail' passed empty list"
#define LERR_BUILTIN_LOOKUP "Unknown function"
#define LERR_ENV_GET        "Unbound symbol: %s"
#define LERR_DEF_ARITY      "Function 'def' cannot define non-symbol"
#define LERR_TYPE "Function '%s' passed incorrect type. %s != %s"

typedef char bool;
typedef struct lval lval;
typedef struct lenv lenv;
typedef struct sexp sexp;
typedef struct lambda lambda;
typedef struct lval*(lbuiltin)(struct lenv*, struct lval*);

struct sexp {
  int count;
  lval** cell;
};

struct lambda {
  lenv* env;
  lval* formals;
  lval* body;
};

struct lval {
  int type;
  union {
    long num;
    char* err;
    char* sym;
    char* str;
    sexp sexp;
    lbuiltin* builtin;
    lambda lambda;
  } v;
};

struct lenv {
  lenv* parent;
  int count;
  char** syms;
  lval** vals;
};

#define L_COUNT(lval)     (lval)->v.sexp.count
#define L_CELL(lval)      (lval)->v.sexp.cell
#define L_TYPE(lval)      (lval)->type
#define L_CFUN(lval)      (lval)->v.builtin
#define L_LAM(lval)       (lval)->v.lambda
#define L_NUM(lval)       (lval)->v.num
#define L_ERR(lval)       (lval)->v.err
#define L_SYM(lval)       (lval)->v.sym
#define L_STR(lval)       (lval)->v.str

#define L_ENV(lval)       (lval)->v.lambda.env
#define L_FORM(lval)      (lval)->v.lambda.formals
#define L_BODY(lval)      (lval)->v.lambda.body
#define L_FORM_N(lval, n) L_CELL_N((lval)->v.lambda.formals, n)

// child accessors
#define L_CELL_N(lval, n) (lval)->v.sexp.cell[(n)]
#define L_TYPE_N(lval, n) L_CELL_N(lval, n)->type
#define L_COUNT_N(lval, n) L_COUNT(L_CELL_N(lval, n))
#define FOREACH_SEXP(i, e) for (int i = 0, _max = L_COUNT(e); i < _max; ++i)

// envs
#define E_PARENT(e)   (e)->parent
#define E_COUNT(e)    (e)->count
#define E_SYMS(e)     (e)->syms
#define E_VALS(e)     (e)->vals
#define E_SYM_N(e, i) (e)->syms[i]
#define E_VAL_N(e, i) (e)->vals[i]
#define FOREACH_ENV(i, e) for (int i = 0, _max = E_COUNT(e); i < _max; ++i)
#define DEFINE_LOCAL  0
#define DEFINE_GLOBAL 1

// utilities
#define RETURN_ERR(s, msg) if (1) { lval_del(s); return lval_err(msg); }
#define LOOKUP(a, b) strcmp((a), (b)) == 0
#define SUBSTR(a, b) strstr((a), (b))
#define BUILTIN(n) lval* builtin_##n(lenv* e, lval* a)

#define CHECK_ARITY(f, v, n)                           \
  if (L_COUNT(v) != n) {                               \
    lval_del(v);                                       \
    return lval_err(LERR_ARITY, (f), (n), L_COUNT(v)); \
  }

#define CHECK_TYPE(f, v, n, t)                      \
  if (L_TYPE_N(v, n) != (t)) {                      \
    lval_del(a);                                    \
    return lval_err(LERR_TYPE, (f),                 \
                    lval_type_name(L_TYPE_N(v, n)), \
                    lval_type_name(t));             \
  }

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

/* ch14.c */
lval *lval_new(void);
lval *lval_err(char *fmt, ...);
lval *lval_fun(lbuiltin *func);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_num(long x);
lval *lval_qexp(void);
lval *lval_sexp(void);
lval *lval_str(char *s);
lval *lval_sym(char *s);
lval *lval_copy(lval *v);
void lval_del(lval *v);
lval *lval_add(lval *v, lval *x);
lval *lval_call(lenv *e, lval *f, lval *a);
lval *lval_cons(lval *x, lval *xs);
int lval_eq(lval *x, lval *y);
lval *lval_join(lval *x, lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lenv *lenv_new(void);
lenv *lenv_copy(lenv *e);
lval *lenv_add(lenv *e, lval *a, bool global);
void lenv_def(lenv *e, lval *k, lval *v);
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
lval *lval_read_str(mpc_ast_t *t);
lval *builtin_add(lenv *e, lval *a);
lval *builtin_cons(lenv *e, lval *a);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_exp(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_if(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_lambda(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_load(lenv *e, lval *a);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_max(lenv *e, lval *a);
lval *builtin_min(lenv *e, lval *a);
lval *builtin_mod(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
char *lval_type_name(int t);
void lval_print(lval *v);
void lval_print_expr(lval *v, char open, char close);
void lval_print_str(lval *v);
void lval_println(lval *v);
int main(int argc, char **argv);
/* DONE */

/*
 * Static Data
 */

static mpc_parser_t* Lispy;

/*
 * Constructors / Destructors
 */

lval* lval_new() {
  lval* v = malloc(sizeof(lval));
  L_TYPE(v) = -1;
  L_NUM(v) = 0xFFFFFFFF;
  return v;
}

lval* lval_err(char* fmt, ...) {
  va_list va;

  lval* v = lval_new();
  L_TYPE(v) = LVAL_ERR;

  va_start(va, fmt);
  vasprintf(&L_ERR(v), fmt, va);
  va_end(va);

  return v;
}

lval* lval_fun(lbuiltin* func) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_FUN;
  L_CFUN(v) = func;
  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = lval_new();
  L_TYPE(v) = LVAL_LAM;
  L_ENV(v)  = lenv_new();
  L_FORM(v) = formals;
  L_BODY(v) = body;
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

lval* lval_str(char* s) {
  lval* v = lval_err(s);
  L_TYPE(v) = LVAL_STR;
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
    L_CFUN(x) = L_CFUN(v);
    break;
  case LVAL_LAM:
    L_ENV(x)  = lenv_copy(L_ENV(v));
    L_FORM(x) = lval_copy(L_FORM(v));
    L_BODY(x) = lval_copy(L_BODY(v));
    break;
  case LVAL_NUM:
    L_NUM(x) = L_NUM(v);
    break;
  case LVAL_ERR:
    L_ERR(x) = strdup(L_ERR(v));
    break;
  case LVAL_SYM:
    L_SYM(x) = strdup(L_SYM(v));
    break;
  case LVAL_STR:
    L_STR(x) = strdup(L_STR(v));
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
    break;
  case LVAL_FUN:
    break;
  case LVAL_LAM:
    lenv_del(L_ENV(v));
    lval_del(L_FORM(v));
    lval_del(L_BODY(v));
    break;
  case LVAL_SYM:
    free(L_SYM(v));
    break;
  case LVAL_STR:
    free(L_STR(v));
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

lval* lval_call(lenv* e, lval* f, lval* a) {
  if (L_TYPE(f) == LVAL_FUN) return L_CFUN(f)(e, a);

  int given = L_COUNT(a);
  int total = L_COUNT(L_FORM(f));

  while (L_COUNT(a)) {
    if (L_COUNT(L_FORM(f)) == 0) {
      lval_del(a);
      return lval_err(LERR_ARITY, "call formals", given, total);
    }

    lval* sym = lval_pop(L_FORM(f), 0);

    if (LOOKUP(L_SYM(sym), "&")) {
      CHECK_ARITY("& format", L_FORM(f), 1);

      lval* nsym = lval_pop(L_FORM(f), 0);
      lenv_put(L_ENV(f), nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);

      break;
    }

    lval* val = lval_pop(a, 0);

    lenv_put(L_ENV(f), sym, val);

    lval_del(sym);
    lval_del(val);
  }

  lval_del(a);

  if (L_COUNT(L_FORM(f)) > 0 && LOOKUP(L_SYM(L_FORM_N(f, 0)), "&")) {
    CHECK_ARITY("call format & arg", L_FORM(f), 2);

    lval_del(lval_pop(L_FORM(f), 0));

    lval* sym = lval_pop(L_FORM(f), 0);
    lval* val = lval_qexp();

    lenv_put(L_ENV(f), sym, val);

    lval_del(sym);
    lval_del(val);
  }

  if (L_COUNT(L_FORM(f)) == 0) {
    E_PARENT(L_ENV(f)) = e;
    return builtin_eval(L_ENV(f), lval_add(lval_sexp(), lval_copy(L_BODY(f))));
  } else {
    return lval_copy(f);
  }
}

lval* lval_cons(lval* x, lval *xs) {
  size_t size = L_COUNT(xs);
  L_COUNT(xs)++;
  L_CELL(xs) = realloc(L_CELL(xs), sizeof(lval*) * (size+1));
  memmove(&L_CELL_N(xs, 1), &L_CELL_N(xs, 0), sizeof(lval*) * size);
  L_CELL_N(xs, 0) = x;
  return xs;
}

int lval_eq(lval* x, lval* y) {
  if (L_TYPE(x) != L_TYPE(y)) return 0;

  switch (L_TYPE(x)) {
  case LVAL_NUM:
    return L_NUM(x) == L_NUM(y);
  case LVAL_ERR:
    return LOOKUP(L_ERR(x), L_ERR(y));
  case LVAL_SYM:
    return LOOKUP(L_SYM(x), L_SYM(y));
  case LVAL_STR:
    return LOOKUP(L_STR(x), L_STR(y));
  case LVAL_FUN:
    return L_CFUN(x) == L_CFUN(y);
  case LVAL_LAM:
    return lval_eq(L_FORM(x), L_FORM(y)) && lval_eq(L_BODY(x), L_BODY(y));
  case LVAL_QEXP:
  case LVAL_SEXP:
    if (L_COUNT(x) != L_COUNT(y)) return 0;
    FOREACH_SEXP(i, x) {
      if (!lval_eq(L_CELL_N(x, i), L_CELL_N(y, i))) return 0;
    }
    return 1;
    break;
  default:
    printf("Unknown lval type: %d", L_TYPE(x));
    return 0;
    break;
  }
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
  E_COUNT(e)  = 0;
  E_PARENT(e) = NULL;
  E_SYMS(e)   = NULL;
  E_VALS(e)   = NULL;
  return e;
}

lenv* lenv_copy(lenv* e) {
  lenv* n = lenv_new();
  E_PARENT(n) = E_PARENT(e);
  E_COUNT(n)  = E_COUNT(e);
  E_SYMS(n)   = malloc(sizeof(char*) * E_COUNT(n));
  E_VALS(n)   = malloc(sizeof(lval*) * E_COUNT(n));

  FOREACH_ENV(i, n) {
    E_SYM_N(n, i) = strdup(E_SYM_N(e, i));
    E_VAL_N(n, i) = lval_copy(E_VAL_N(e, i));
  }

  return n;
}

lval* lenv_add(lenv* e, lval* a, bool global) {
  lval* syms = L_CELL_N(a, 0);

  CHECK_TYPE("def", a, 0, LVAL_QEXP);
  if (L_COUNT(syms) != L_COUNT(a)-1)   RETURN_ERR(a, LERR_DEF_ARITY);
  FOREACH_SEXP(i, syms) {
    CHECK_TYPE("def (syms)", syms, i, LVAL_SYM);
  }

  FOREACH_SEXP(i, syms) {
    if (global) {
      lenv_put(e, L_CELL_N(syms, i), L_CELL_N(a, i+1));
    } else {
      lenv_def(e, L_CELL_N(syms, i), L_CELL_N(a, i+1));
    }
  }

  lval_del(a);

  return lval_sexp();
}

void lenv_def(lenv* e, lval* k, lval* v) {
  while (E_PARENT(e)) e = E_PARENT(e);

  lenv_put(e, k, v);
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

  if (E_PARENT(e)) {
    return lenv_get(E_PARENT(e), k);
  } else {
    return lval_err(LERR_ENV_GET, L_SYM(k));
  }
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
  do {
    FOREACH_ENV(i, e) {
      printf("  %s = ", E_SYM_N(e, i));
      lval_println(E_VAL_N(e, i));
    }

    if (E_PARENT(e)) printf("-----\n");

    e = E_PARENT(e);
  } while (e);
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
  lenv_add_builtin(e, "+",      builtin_add);
  lenv_add_builtin(e, "cons",   builtin_cons);
  lenv_add_builtin(e, "def",    builtin_def);
  lenv_add_builtin(e, "/",      builtin_div);
  lenv_add_builtin(e, "eval",   builtin_eval);
  lenv_add_builtin(e, "^",      builtin_exp);
  lenv_add_builtin(e, "head",   builtin_head);
  lenv_add_builtin(e, "join",   builtin_join);
  lenv_add_builtin(e, "lambda", builtin_lambda);
  lenv_add_builtin(e, "len",    builtin_len);
  lenv_add_builtin(e, "list",   builtin_list);
  lenv_add_builtin(e, "load",   builtin_load);
  lenv_add_builtin(e, "max",    builtin_max);
  lenv_add_builtin(e, "min",    builtin_min);
  lenv_add_builtin(e, "%",      builtin_mod);
  lenv_add_builtin(e, "*",      builtin_mul);
  lenv_add_builtin(e, "=",      builtin_put);
  lenv_add_builtin(e, "-",      builtin_sub);
  lenv_add_builtin(e, "tail",   builtin_tail);

  lenv_add_builtin(e, ">",      builtin_gt);
  lenv_add_builtin(e, "<",      builtin_lt);
  lenv_add_builtin(e, ">=",     builtin_ge);
  lenv_add_builtin(e, ">=",     builtin_le);
  lenv_add_builtin(e, "==",     builtin_eq);
  lenv_add_builtin(e, "!=",     builtin_ne);
  lenv_add_builtin(e, "if",     builtin_if);
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
  lval* result;

  switch (L_TYPE(f)) {
  case LVAL_FUN:
    result = L_CFUN(f)(e, v);
    lval_del(f);
    return result;
    break;
  case LVAL_LAM:
    result = lval_call(e, f, v);
    return result;
    break;
  default:
    lval_del(f);
    RETURN_ERR(v, LERR_BAD_SEXP);
  }

}

lval* lval_read(mpc_ast_t* t) {
  lval* x = NULL;

  if (SUBSTR(t->tag, "number")) { return lval_read_num(t);      }
  if (SUBSTR(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (SUBSTR(t->tag, "string")) { return lval_read_str(t);      }
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
    if (SUBSTR(child->tag, "comment")){ continue; }
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

lval* lval_read_str(mpc_ast_t* t) {
  t->contents[strlen(t->contents)-1] = '\0'; // UGH
  char * unescaped = mpcf_unescape(strdup(t->contents+1));
  lval* str = lval_str(unescaped);
  free(unescaped);
  return str;
}

/*
 * Builtins
 */

BUILTIN(add) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) += n);
  return result;
}

BUILTIN(cons) {
  CHECK_ARITY("cons", a, 2);
  CHECK_TYPE("cons", a, 1, LVAL_QEXP);

  lval* x = lval_pop(a, 0);
  lval* s = lval_pop(a, 0);

  return lval_cons(x, s);
}

BUILTIN(def) {
  return lenv_add(e, a, DEFINE_LOCAL);
}

BUILTIN(div) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) /= n);
  return result;
}

BUILTIN(eval) {
  CHECK_ARITY("eval", a, 1);
  CHECK_TYPE("eval", a, 0, LVAL_QEXP);

  lval* x = lval_take(a, 0);
  L_TYPE(x) = LVAL_SEXP; // TODO: ARGH

  return lval_eval(e, x);
}

BUILTIN(eq) {
  CHECK_ARITY("==", a, 2);

  lval *m = lval_pop(a, 0);
  lval *n = lval_pop(a, 0);

  int r = lval_eq(m, n);

  lval_del(a);

  return lval_num(r);
}

BUILTIN(exp) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) ^= n);
  return result;
}

BUILTIN(ge) {
  CHECK_ARITY(">=", a, 2);
  CHECK_FOR_NUMBERS(a);
  return lval_num(L_CELL_N(a, 0) >= L_CELL_N(a, 1));
}

BUILTIN(gt) {
  CHECK_ARITY(">", a, 2);
  CHECK_FOR_NUMBERS(a);
  return lval_num(L_CELL_N(a, 0) > L_CELL_N(a, 1));
}

BUILTIN(head) {
  CHECK_ARITY("head", a, 1);
  CHECK_TYPE("head", a, 0, LVAL_QEXP);
  if (L_COUNT(L_CELL_N(a, 0)) == 0) RETURN_ERR(a, LERR_HEAD_EMPTY);

  lval* v = lval_take(a, 0);

  while(L_COUNT(v) > 1) { // WTF? this seems REALLY bad!
    lval_del(lval_pop(v, 1));
  }

  return v;
}

BUILTIN(if) {
  CHECK_ARITY("if", a, 3);
  CHECK_TYPE("if", a, 0, LVAL_NUM);
  CHECK_TYPE("if", a, 1, LVAL_QEXP);
  CHECK_TYPE("if", a, 2, LVAL_QEXP);

  lval* c = lval_pop(a, 0);
  lval* t = lval_pop(a, 0);
  lval* f = lval_pop(a, 0);
  lval* r;

  L_TYPE(t) = LVAL_SEXP;
  L_TYPE(f) = LVAL_SEXP;

  if (L_NUM(c)) {
    r = lval_eval(e, t);
  } else {
    r = lval_eval(e, f);
  }

  lval_del(a);

  return r;
}

BUILTIN(join) {
  FOREACH_SEXP(i, a) {
    CHECK_TYPE("join", a, i, LVAL_QEXP);
  }

  lval* x = lval_pop(a, 0);

  while (L_COUNT(a)) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);

  return x;
}

BUILTIN(lambda) {
  CHECK_ARITY("lambda", a, 2);
  CHECK_TYPE("lambda", a, 0, LVAL_QEXP);
  CHECK_TYPE("lambda", a, 1, LVAL_QEXP);

  FOREACH_SEXP(i, a) {
    CHECK_TYPE("lambda args", L_CELL_N(a, 0), i, LVAL_SYM); // TODO: refactor
  }

  lval* formals = lval_pop(a, 0);
  lval* body    = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

BUILTIN(le) {
  CHECK_ARITY("<=", a, 2);
  CHECK_FOR_NUMBERS(a);
  return lval_num(L_CELL_N(a, 0) <= L_CELL_N(a, 1));
}

BUILTIN(len) {
  return lval_num(L_COUNT_N(a, 0));
}

BUILTIN(list) {
  L_TYPE(a) = LVAL_QEXP; // TODO: these are all mutating lval. HORRIBLE.

  return a;
}

BUILTIN(load) {
  CHECK_ARITY("load", a, 1);
  CHECK_TYPE("load", a, 0, LVAL_STR);

  mpc_result_t r;
  if (mpc_parse_contents(L_STR(L_CELL_N(a, 0)), Lispy, &r)) {
    lval* expr = lval_read(r.output);
    mpc_ast_delete(r.output);

    while (L_COUNT(expr)) {
      lval *x = lval_pop(expr, 0);
      x = lval_eval(e, x);

      if (L_TYPE(x) == LVAL_ERR) lval_println(x);
      lval_del(x);
    }

    lval_del(expr);
    lval_del(a);

    return lval_sexp();
  } else {
    char* err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    lval* err = lval_err("Could not load library %s", err_msg);
    free(err_msg);
    lval_del(a);

    return err;
  }
}

BUILTIN(lt) {
  CHECK_ARITY("<", a, 2);
  CHECK_FOR_NUMBERS(a);
  return lval_num(L_CELL_N(a, 0) < L_CELL_N(a, 1));
}

BUILTIN(max) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, long m = L_NUM(result); L_NUM(result) = n >= m ? n : m);
  return result;
}

BUILTIN(min) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, long m = L_NUM(result); L_NUM(result) = n <= m ? n : m);
  return result;
}

BUILTIN(mod) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) %= n);
  return result;
}

BUILTIN(mul) {
  CHECK_FOR_NUMBERS(a);
  FOREACH_NUMBER(a, result, n, L_NUM(result) *= n);
  return result;
}

BUILTIN(ne) {
  CHECK_ARITY("!=", a, 2);

  lval *m = lval_pop(a, 0);
  lval *n = lval_pop(a, 0);

  int r = lval_eq(m, n);

  lval_del(a);

  return lval_num(!r);
}

BUILTIN(put) {
  return lenv_add(e, a, DEFINE_GLOBAL);
}

BUILTIN(sub) {
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

BUILTIN(tail) {
  CHECK_ARITY("tail", a, 1);
  CHECK_TYPE("tail", a, 0, LVAL_QEXP);
  if (L_COUNT(L_CELL_N(a, 0)) == 0) RETURN_ERR(a, LERR_TAIL_EMPTY);

  lval* v = lval_take(a, 0);

  lval_del(lval_pop(v, 0)); // WTF? This seems really bad!

  return v;
}

/*
 * Main & Side effect free
 */

char* lval_type_name(int t) {
  switch (t) {
  case LVAL_FUN: return "builtin";
  case LVAL_LAM: return "lambda";
  case LVAL_NUM: return "number";
  case LVAL_ERR: return "error";
  case LVAL_SYM: return "symbol";
  case LVAL_STR: return "string";
  case LVAL_SEXP: return "sexp";
  case LVAL_QEXP: return "qexp";
  default: return "unknown";
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
  case LVAL_LAM:
    printf("(lambda ");
    lval_print(L_FORM(v));
    putchar(' ');
    lval_print(L_BODY(v));
    putchar(')');
    break;
  case LVAL_SYM:
    printf("%s", L_SYM(v));
    break;
  case LVAL_STR:
    lval_print_str(v);
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

    if (i != _max-1) {
      putchar(' ');
    }
  }

  putchar(close);
}

void lval_print_str(lval* v) {
  char * escaped = mpcf_escape(strdup(L_STR(v)));
  printf("\"%s\"", escaped);
  free(escaped);
}


void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

int main(int argc, char** argv) {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* String   = mpc_new("string");
  mpc_parser_t* Comment  = mpc_new("comment");
  mpc_parser_t* Sexp     = mpc_new("sexp");
  mpc_parser_t* Qexp     = mpc_new("qexp");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_result_t r;

  Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT, "                                \
            number   : /-?[0-9]+(\\.[0-9]+)?/;                  \
            symbol   : /[a-zA-Z0-9_+*\\/\\\\=<>!&-]+/;          \
            string   : /\"(\\\\.|[^\"])*\"/;                    \
            comment  : /;[^\\r\\n]*/;                           \
            sexp     : '(' <expr>* ')';                         \
            qexp     : '{' <expr>* '}';                         \
            expr     : <number> | <symbol> | <string> | <comment> | <sexp> | <qexp>; \
            lispy    : /^/ <expr>* /$/;                         ",
            Number, Symbol, String, Comment, Sexp, Qexp, Expr, Lispy);

  lenv* env = lenv_new();
  lenv_add_builtins(env);

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      lval* args = lval_add(lval_sexp(), lval_str(argv[i]));
      lval* x = builtin_load(env, args);
      if (L_TYPE(x) == LVAL_ERR) lval_println(x);
      lval_del(x);
    }
  }

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
  mpc_cleanup(8, Number, Symbol, String, Comment, Sexp, Qexp, Expr, Lispy);

  return 0;
}
