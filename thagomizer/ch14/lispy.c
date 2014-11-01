#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_EMPTY(func, args) \
  LASSERT(args, ((args)->cell[0]->count != 0), "Function %s passed the empty list!", func)

#define LASSERT_ARG_COUNT(func, args, expected) \
  LASSERT(args, ((args)->count == (expected)), "Function %s passed incorrect number of arguments. Got %i, Expected %i.", func, (args)->count, (expected))

/** Forward Declarations **/
struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

/** Lisp Value **/
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR, LVAL_BOOL };

/** Types **/

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
  int type;

  /* Basic */
  long num;
  char* err;
  char* sym;
  char* str;

  /* Function */
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;

  /* Expression */
  int count;
  lval** cell;
};

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

/** Forward Function Declaration **/
/* ch14/lispy.c */
lval *lval_num(long x);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_str(char *s);
lval *lval_fun(lbuiltin func);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_bool(int num);
void lval_del(lval *v);
lval *lval_copy(lval *v);
lval *lval_eq(lval *left, lval *right);
lval *lval_add(lval *v, lval *x);
lval *lval_join(lval *x, lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
void lval_expr_print(lval *v, char open, char close);
void lval_print_str(lval *v);
void lval_print(lval *v);
void lval_println(lval *v);
char *ltype_name(int t);
lenv *lenv_new(void);
void lenv_del(lenv *e);
lval *lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval *v);
lenv *lenv_copy(lenv *e);
void lenv_def(lenv *e, lval *k, lval *v);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_cons(lenv *e, lval *a);
lval *builtin_len(lenv *e, lval *a);
lval *builtin_exit(lenv *e, lval *a);
lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_add(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);
lval *builtin_pow(lenv *e, lval *a);
lval *builtin_mod(lenv *e, lval *a);
lval *builtin_min(lenv *e, lval *a);
lval *builtin_max(lenv *e, lval *a);
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_lambda(lenv *e, lval *a);
lval *builtin_if(lenv *e, lval *a);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);
lval *lval_call(lenv *e, lval *f, lval *a);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read_str(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
/* DONE */

/** Actual Code **/

/* Construct a pointer to a new Number lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a user defined function */
lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  v->builtin = NULL;

  v->env = lenv_new();

  v->formals = formals;
  v->body = body;
  return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* Allocate 512 bytes of space */
  v->err = malloc(512);

  /* printf into the error string with a maximum of 511 characters */
  vsnprintf(v->err, 511, fmt, va);


  /* Reallocate to number of bytes actually used */
  v->err = realloc(v->err, strlen(v->err) + 1);

  /* Cleanup our va list */
  va_end(va);

  return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_str(char* s) { 
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_STR;
  v->str  = malloc(strlen(s) + 1);
  strcpy(v->str, s);
  return v;
}

lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* A pointer to a new empty Qexpr lval */
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_bool(int num) { 
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_BOOL;
  v->num = num;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
  case LVAL_NUM: break;
  case LVAL_BOOL: break;
  case LVAL_FUN:
    if (!v->builtin) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
    break;
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;
  case LVAL_STR: free(v->str); break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  }

  free(v);
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {

  /* Copy Functions and Numbers Directly */
  case LVAL_FUN:
    if (v->builtin) {
      x->builtin = v->builtin;
    } else {
      x->builtin = NULL;
      x->env = lenv_copy(v->env);
      x->formals = lval_copy(v->formals);
      x->body = lval_copy(v->body);
    }
    break;
  case LVAL_NUM:
  case LVAL_BOOL:
    x->num = v->num;
    break;

  /* Copy Strings using malloc and strcpy */
  case LVAL_ERR:
    x->err = malloc(strlen(v->err) + 1);
    strcpy(x->err, v->err);
    break;
  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym) + 1);
    strcpy(x->sym, v->sym);
    break;
  case LVAL_STR:
    x->str = malloc(strlen(v->str) + 1);
    strcpy(x->str, v->str);
    break;

  /* Copy Lists by copying each sub-expression */
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval*) * x->count);
    for (int i = 0; i < x->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
    break;
  }

  return x;
}

lval* lval_eq(lval* left, lval* right) { 
  if (left->type != right->type) { 
    return lval_bool(0); 
  }

  switch(left->type) { 
  case LVAL_NUM:
  case LVAL_BOOL:
    if (left->num == right->num) { 
      return lval_bool(1);
    } else {
      return lval_bool(0);
    }
  case LVAL_SYM:
    if (strcmp(left->sym, right->sym) == 0) { 
      return lval_bool(1);
    } else {
      return lval_bool(0);
    }
  case LVAL_STR:
    if (strcmp(left->str, right->str) == 0) { 
      return lval_bool(1);
    } else {
      return lval_bool(0);
    }
  case LVAL_FUN:
    if (left->builtin == right->builtin) {
      return lval_bool(1);
    } else {
      if ((lval_eq(left->formals, right->formals)->num == 0) &&
        (lval_eq(left->body, right->body)->num == 0))
      return lval_bool(0);
    }
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    if (left->count != right->count) { 
      return lval_bool(0);
    } 

    for (int i = 0; i < left->count; i++) { 
      if ((lval_eq(left->cell[i], right->cell[i]))->num == 0) {
        return lval_bool(0);
      }
    }
    return lval_bool(1);
  }

  return lval_bool(0);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
}

lval* lval_join(lval* x, lval* y) {
  /* For each cell in 'y' add it to 'x' */
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* Delete the empty 'y' and return 'x' */
  lval_del(y);
  return x;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count-i - 1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print_str(lval* v) { 
  /* Make a Copy of the string */
  char* escaped = malloc(strlen(v->str) + 1);
  strcpy(escaped, v->str);

  /* Pass it through the escape function */
  escaped = mpcf_escape(escaped);

  /* Print it between " characters */
  printf("\"%s\"", escaped);

  /* free the copied string */
  free(escaped);
}

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_FUN:
    if (v->builtin) {
      printf("<builtin>");
    } else {
      printf("(\\ ");
      lval_print(v->formals);
      putchar(' ');
      lval_print(v->body);
      putchar(')');
    }
    break;
  case LVAL_NUM:
    printf("%li", v->num);
    break;
  case LVAL_BOOL:
    if (v->num == 0) { 
      printf("false");
    } else {
      printf("true");
    }
    break;
  case LVAL_ERR:
    printf("Error: %s", v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
    break;
  case LVAL_STR:
    lval_print_str(v);
    break;
  case LVAL_SEXPR:
    lval_expr_print(v, '(', ')');
    break;
  case LVAL_QEXPR:
    lval_expr_print(v, '{', '}');
    break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

char* ltype_name(int t) {
  switch(t) {
  case LVAL_FUN:
    return "Function";
  case LVAL_NUM:
    return "Number";
  case LVAL_BOOL:
    return "Boolean";
  case LVAL_ERR:
    return "Error";
  case LVAL_SYM:
    return "Symbol";
  case LVAL_STR:
    return "String";
  case LVAL_SEXPR:
    return "S-Expression";
  case LVAL_QEXPR:
    return "Q-Expression";
  default: return "Unknown";
  }
}

/* Lisp Environment */

struct lenv {
  lenv* parent;
  int count;
  char** syms;
  lval** vals;
};

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->parent = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}


lval* lenv_get(lenv* e, lval* k) {

  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
  }

  /* If no symbol found check parent or return error */
  if (e->parent) {
    return lenv_get(e->parent, k);
  } else {
    return lval_err("Unbound Symbol '%s'", k->sym);
  }
}

void lenv_put(lenv* e, lval* k, lval* v) {

  /* Iterate over all items in environment */
  /* This is to see if variable already exists */
  for (int i = 0; i < e->count; i++) {

    /* If variable is found delete item at that position */
    /* And replace with variable supplied by user */
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  /* If no existing entry found then allocate space for new entry */
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  /* Copy contents of lval and symbol string into new location */
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

lenv* lenv_copy(lenv* e) {
  lenv* new = malloc(sizeof(lenv));
  new->parent = e->parent;
  new->count = e->count;
  new->syms = malloc(sizeof(char*) * new->count);
  new->vals = malloc(sizeof(lval*) * new->count);

  for (int i= 0; i < e->count; i++) {
    new->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(new->syms[i], e->syms[i]);
    new->vals[i] = lval_copy(e->vals[i]);
  }
  return new;
}

void lenv_def(lenv* e, lval* k, lval* v) {
  while (e->parent) {
    e = e->parent;
  }

  lenv_put(e, k, v);
}

/* Builtins */

lval* builtin_load(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("load", a, 1);
  LASSERT_TYPE("load", a, 0, LVAL_STR);

  /* Parse File given by string name */
  mpc_result_t r;
  if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {
    /* Read contents */
    lval* expr = lval_read(r.output);
    mpc_ast_delete(r.output);

    /* Evaluate each Expression */
    while (expr->count) { 
      lval* x = lval_eval(e, lval_pop(expr, 0));
      
      /* If Evaluation leads to error print it */
      if (x->type == LVAL_ERR) { 
        lval_println(x);
      }
      lval_del(x);
    }

    /* Delete expressions and arguments */
    lval_del(expr);
    lval_del(a);

    /* Return empty list */
    return lval_sexpr();

  } else {
    /* Get Parse Error as String */
    char* err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    /* Create new error message using it */
    lval* err = lval_err("Could not load Library %s", err_msg);
    free(err_msg);
    lval_del(a);

    /* Cleanup and return error */
    return err;
  }
}

lval* builtin_print(lenv* e, lval* a) { 
  /* Print each argument followed by a space */
  for (int i = 0; i < a->count; i++) { 
    lval_print(a->cell[i]);
    putchar(' ');
  }

  /* Print a newline and delete arguments */
  putchar('\n');
  lval_del(a);

  return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("error", a, 1);
  LASSERT_TYPE("error", a, 0, LVAL_STR);

  /* Construct Error from first argument */
  lval* err = lval_err(a->cell[0]->str);

  /* Delete arguments and return */
  lval_del(a);
  return err;
}

lval* builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("head", a, 1);
  LASSERT_EMPTY("head", a);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);

  lval* v = lval_take(a, 0);

  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_EMPTY("tail", a);

  lval* v = lval_take(a, 0);

  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_eval(lenv *e, lval *a) {
  LASSERT_ARG_COUNT("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv *e, lval *a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("join", a, i, LVAL_QEXPR);
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lenv *e, lval *a) {
  LASSERT_EMPTY("cons", a);
  LASSERT_ARG_COUNT("cons", a, 2);
  LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

  lval* v = lval_pop(a, 0);   /* value */
  lval* q = lval_pop(a, 0);   /* list I HATE THAT POP MUTATES*/

  lval* result = lval_qexpr();
  result = lval_add(result, v);

  result = lval_join(result, q);

  return result;
}

lval* builtin_len(lenv *e, lval *a) {
  LASSERT_ARG_COUNT("len", a, 1);

  lval* v = lval_take(a, 0);

  return lval_num(v->count);
}

lval* builtin_exit(lenv* e, lval* a) {
  exit(0);
}


lval* builtin_op(lenv* e, lval* a, char* op) {
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE(op, a, i, LVAL_NUM);
  }

  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no arguments and subtraction: perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    /* Perform operation */
    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }
    if (strcmp(op, "-") == 0) {
      x->num -= y->num ;
    }
    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }
    if (strcmp(op, "^") == 0) {
      x->num = pow(x->num, y->num);
    }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division By Zero!");
        break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division By Zero!");
        break;
      }
      x->num = x->num % y->num;
    }
    if (strcmp(op, "min") == 0) {
      x->num = x->num <= y->num ? x->num : y->num;
    }
    if (strcmp(op, "max") == 0) {
      x->num = x->num >= y->num ? x->num : y->num;
    }

    /* Delete the element now finished with */
    lval_del(y);
  }

  /* Delete input expresison and return result */
  lval_del(a);

  return x;
}

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_pow(lenv* e, lval* a) {
  return builtin_op(e, a, "^");
}

lval* builtin_mod(lenv* e, lval* a) {
  return builtin_op(e, a, "%");
}

lval* builtin_min(lenv* e, lval* a) {
  return builtin_op(e, a, "min");
}

lval* builtin_max(lenv* e, lval* a) {
  return builtin_op(e, a, "max");
}

/* Functions */

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

  lval* syms = a->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot define non-symbol. Got %s, Expected %.",
            func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }

  LASSERT(a, (syms->count == a->count - 1),
          "Function '%s' passed too many arguments for symbols. Got %i, Expected %i.", func, syms->count, a->count-1);

  for (int i = 0; i < syms->count; i++) {
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i+1]);
    }
    if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

  }

  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
  return builtin_var(e, a, "=");
}

lval* builtin_lambda(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type = LVAL_SYM),
            "Cannot define non-symbol. Got %s, Expected %s.",
            ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }

  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_ARG_COUNT("if", a, 3);

  lval* condition = lval_pop(a, 0);
  lval* t_branch  = lval_pop(a, 0);
  lval* f_branch  = lval_pop(a, 0);

  t_branch->type = LVAL_SEXPR;
  f_branch->type = LVAL_SEXPR;

  lval* result;
  if (condition->num) {
    result = lval_eval(e, t_branch);
  } else {
    result = lval_eval(e, f_branch);
  }

  lval_del(a);
  return result;
}

lval* builtin_gt(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("gt", a, 2);
  LASSERT_TYPE("gt", a, 0, LVAL_NUM);
  LASSERT_TYPE("gt", a, 1, LVAL_NUM);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result;
  if (left->num > right->num) {
    result = lval_bool(1);
  } else {
    result = lval_bool(0);
  }

  return result;
}

lval* builtin_ge(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("ge", a, 2);
  LASSERT_TYPE("ge", a, 0, LVAL_NUM);
  LASSERT_TYPE("ge", a, 1, LVAL_NUM);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result;
  if (left->num >= right->num) {
    result = lval_bool(1);
  } else {
    result = lval_bool(0);
  }

  return result;
}

lval* builtin_lt(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("lt", a, 2);
  LASSERT_TYPE("lt", a, 0, LVAL_NUM);
  LASSERT_TYPE("lt", a, 1, LVAL_NUM);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result;
  if (left->num < right->num) { 
    result = lval_bool(1);
  } else {
    result = lval_bool(0);
  }

  return result;
}

lval* builtin_le(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("le", a, 2);
  LASSERT_TYPE("le", a, 0, LVAL_NUM);
  LASSERT_TYPE("le", a, 1, LVAL_NUM);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result;
  if (left->num <= right->num) { 
    result = lval_bool(1);
  } else {
    result = lval_bool(0);
  }

  return result;
}

lval* builtin_eq(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("eq", a, 2);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result = lval_eq(left, right);

  lval_del(a);

  return result;
}

lval* builtin_ne(lenv* e, lval* a) { 
  LASSERT_ARG_COUNT("eq", a, 2);

  lval* left  = lval_pop(a, 0);
  lval* right = lval_pop(a, 0);

  lval* result = lval_eq(left, right);

  if (result->num == 0) {
    result->num = 1;
  } else {
    result->num = 0;
  }

  lval_del(a);

  return result;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  /* If Builtin then simply apply that */
  if (f->builtin) {
    return f->builtin(e, a);
  }

  /* Record Argument Counts */
  int given = a->count;
  int total = f->formals->count;


  /* While arguments still remain to be processed */
  while (a->count) {

    /* If we've ran out of formal argumens to bind */
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. Got %i, Expected %i.", given, total);
    }

    /* Pop the first symbol from the formals */
    lval* sym = lval_pop(f->formals, 0);

    /* Special case to deal with '&' */
    if (strcmp(sym->sym, "&") == 0) {

      /* Ensure '&' is followed by another symbol */
      if (f->formals->count != 1) { 
        lval_del(a);
        return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
      }

      /* Next formal should be bound to remaining arguments */
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }

    /* Pop the next argument from the list */
    lval* val = lval_pop(a, 0);

    /* Bind a copy into the function's environment */
    lenv_put(f->env, sym, val);

    /* Delete symbol and value */
    lval_del(sym);
    lval_del(val);
  }

  /* Argument list is now bound so can be cleaned up */
  lval_del(a);

  /* If '&' remains in formal list it should be bound to empty list */
  if (f->formals->count > 0 && 
      strcmp(f->formals->cell[0]->sym, "&") == 0) {
    
    if (f->formals->count != 2) { 
      return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
    }

    lval_del(lval_pop(f->formals, 0));

    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();

    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  /* If all formals have been bound evaluate */
  /* WTF WHY DOES THIS statement mean that all the formals are bound? */
  if (f->formals->count == 0) {

    /* Set Function Environment parent to current evaluation Enviroment */
    f->env->parent = e;

    /* Evaluate and return */
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    /* Otherwise return partial evaluated function */
    return lval_copy(f);
  }
}


void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "len", builtin_len);

  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "^", builtin_pow);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "min", builtin_min);
  lenv_add_builtin(e, "max", builtin_max);

  /* Variable Functions */
  lenv_add_builtin(e, "\\",  builtin_lambda);
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=", builtin_put);

  /* Conditional Functions */
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "gt", builtin_gt);
  lenv_add_builtin(e, ">",  builtin_gt);
  lenv_add_builtin(e, "ge", builtin_ge);
  lenv_add_builtin(e, ">=", builtin_ge);
  lenv_add_builtin(e, "lt", builtin_lt);
  lenv_add_builtin(e, "<",  builtin_lt);
  lenv_add_builtin(e, "le", builtin_le);
  lenv_add_builtin(e, "<=", builtin_le);
  lenv_add_builtin(e, "eq", builtin_eq);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "ne", builtin_ne);
  lenv_add_builtin(e, "!=", builtin_ne);

  /* Other Functions */
  lenv_add_builtin(e, "exit", builtin_exit);

  /* String Functions */
  lenv_add_builtin(e, "load", builtin_load);
  lenv_add_builtin(e, "error", builtin_error);
  lenv_add_builtin(e, "print", builtin_print);
}

/* Evaluation */
lval* lval_eval_sexpr(lenv* e, lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if(v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  /* Empty Expression */
  if (v->count == 0) {
    return v;
  }

  /* Single Expression */
  if (v->count == 1) {
    return lval_take(v, 0);
  }

  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval* err = lval_err(
      "S-Expression starts with incorrect type. Got %s, Expected %s.",
      ltype_name(f->type), ltype_name(LVAL_FUN));
    lval_del(v);
    lval_del(f);
    return err;
  }

  /* If so call function to get result */
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
  }

  /* All other lvaltypes remain the same */
  return v;
}

/* Reading */
lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read_str(mpc_ast_t* t) { 
  /* Cut off the final quote character */
  t->contents[strlen(t->contents) - 1] = '\0';
  
  /* Copy the string m issing out the first quote character */
  char* unescaped = malloc(strlen(t->contents + 1) + 1);
  strcpy(unescaped, t->contents + 1);

  /* Pass through the unescape function */
  unescaped = mpcf_unescape(unescaped);

  /* Construct a new lval using the string  */
  lval* str = lval_str(unescaped);

  /* Free the string and return */
  free(unescaped);
  return str;
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (strstr(t->tag, "string")) { return lval_read_str(t); }

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    if (strstr(t->children[i]->tag, "comment")) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

int main(int argc, char** argv) {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* String   = mpc_new("string");
  mpc_parser_t* Comment  = mpc_new("comment");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Qexpr    = mpc_new("qexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                 \
            number   :  /-?[0-9]+/ ;                                          \
            symbol   :  /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&+\\%\\^]+/;              \
            comment  :  /;[^\\r\\n]*/;                                        \
            string   :  /\"(\\\\.|[^\"])*\"/ ;                                \
            sexpr    : '(' <expr>* ')' ;                                      \
            qexpr    : '{' <expr>* '}' ;                                      \
            expr     : <number> | <symbol> | <string> | <sexpr> | <qexpr> ;   \
            lispy    : /^/ <expr>* /$/ ;                                      \
            ",
            Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  /* Supplied with list of files */
  if (argc >= 2) {
    
    /* loop over each supplied filename (starting from 1) */
    for (int i = 1; i < argc; i++) { 

      /* Create an argument list with a single argument being the filename */
      lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

      /* Pass to builtin load and get the result */
      lval* x = builtin_load(e, args);

      /* If the result is an error be sure to print it */
      if (x->type == LVAL_ERR) {
        lval_println(x);
      }
      lval_del(x);
    }
  }


  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /*      mpc_ast_print(r.output); */

      lval* x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      /* Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  lenv_del(e);

  mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

  return 0;
}
