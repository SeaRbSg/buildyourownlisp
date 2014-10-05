#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#define LASSERT(args, cond, err) if (!(cond)) {lval_del(args); return lval_err(err); }
#define LEMPTYLIST(args) LASSERT(args, ((args)->cell[0]->count != 0), "Function passed the empty list!")
#define LARGCOUNT(args, numargs) LASSERT(args, ((args)->count == (numargs)), "Function passed the incorrect number of arguments.")

/** Types **/

typedef struct lval {
  int type;
  long num;

  /* Error and Symbol types have some string data */
  char* err;
  char* sym;

  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval** cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/** Forward Declarations **/
lval *lval_num(long x);
lval *lval_err(char *m);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
void lval_del(lval *v);
lval *lval_add(lval *v, lval *x);
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);
void lval_expr_print(lval *v, char open, char close);
void lval_print(lval *v);
void lval_println(lval *v);
lval *lval_eval_sexpr(lval *v);
lval *lval_eval(lval *v);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);
lval *builtin_head(lval *a);
lval *builtin_tail(lval *a);
lval *builtin_list(lval *a);
lval *builtin_eval(lval *a);
lval *builtin_join(lval *a);
lval *builtin_cons(lval *a);
lval *builtin_len(lval *a);
lval *lval_join(lval *x, lval *y);
lval *builtin(lval *a, char *func);
lval *builtin_op(lval *a, char *op);
int main(void);

/** Actual Code **/

/* Construct a pointer to a new Number lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
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

void lval_del(lval* v) {
  switch (v->type) {
  case LVAL_NUM: break;

  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;

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

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

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
    x = lval_add(x, lval_read(t->children[i]));
  }

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

void lval_print(lval* v) {
  switch (v->type) {
  case LVAL_NUM:
    printf("%li", v->num);
    break;
  case LVAL_ERR:
    printf("Error: %s", v->err);
    break;
  case LVAL_SYM:
    printf("%s", v->sym);
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

lval* lval_eval_sexpr(lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
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
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  /* All other lvaltypes remain the same */
  return v;
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift the memory following the item at "i" over the top of it */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_head(lval* a) {
  LARGCOUNT(a, 1);
  LEMPTYLIST(a);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR),
          "Function 'head' passed incorrect type!");

  lval* v = lval_take(a, 0);

  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lval* a) {
  LARGCOUNT(a, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR),
          "Function 'tail', passed incorrect type!");
  LEMPTYLIST(a);

  lval* v = lval_take(a, 0);

  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_init(lval* a) {
  LARGCOUNT(a, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR),
          "Function 'tail', passed incorrect type!");
  LEMPTYLIST(a);

  lval* v = lval_take(a, 0);

  long count = v->count;

  lval* last = lval_pop(v, count - 1);
  lval_del(last);
  return v;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a) {
  LARGCOUNT(a, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR),
          "Function 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_join(lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, (a->cell[i]->type == LVAL_QEXPR),
            "Function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lval* a) {
  LEMPTYLIST(a);
  LARGCOUNT(a, 2);

  lval* v = lval_pop(a, 0);   /* value */
  lval* q = lval_pop(a, 0);   /* list I HATE THAT POP MUTATES*/

  LASSERT(q, (q->type == LVAL_QEXPR),
          "Function 'cons' passed incorrect type.");

  lval* result = lval_qexpr();
  result = lval_add(result, v);

  result = lval_join(result, q);

  return result;
}

lval* builtin_len(lval* a) {
  LARGCOUNT(a, 1);

  lval* v = lval_take(a, 0);

  return lval_num(v->count);
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

lval* builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) {
    return builtin_list(a);
  }
  if (strcmp("head", func) == 0) {
    return builtin_head(a);
  }
  if (strcmp("tail", func) == 0) {
    return builtin_tail(a);
  }
  if (strcmp("join", func) == 0) {
    return builtin_join(a);
  }
  if (strcmp("eval", func) == 0) {
    return builtin_eval(a);
  }
  if (strcmp("cons", func) == 0) {
    return builtin_cons(a);
  }
  if (strcmp("len", func) == 0) {
    return builtin_len(a);
  }
  if (strcmp("init", func) == 0) {
    return builtin_init(a);
  }
  if (strstr("+-/*^%minmax", func)) {
    return builtin_op(a, func);
  }
  lval_del(a);
  return lval_err("Unknown Function!");
}

lval* builtin_op(lval* a, char* op) {

  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
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

int main() {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol   = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Qexpr    = mpc_new("qexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispy    = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                 \
            number   :   /-?[0-9]+/ ;                                         \
            symbol   :   '+' | '-' | '*' | '/' | '%' | '^' | /min/ | /max/ | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" | \"len\" | \"init\" ; \
            sexpr    : '(' <expr>* ')' ;                                      \
            qexpr    : '{' <expr>* '}' ;                                      \
            expr     : <number> | <symbol> | <sexpr> | <qexpr> ;              \
            lispy    : /^/ <expr>* /$/ ;                                      \
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.5");
  puts("Press Crtl-c to Exit\n");

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      mpc_ast_print(r.output);

      lval* x = lval_eval(lval_read(r.output));
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

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  return 0;
}
