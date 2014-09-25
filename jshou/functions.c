#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#include "functions.h"

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));

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
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }

  return lval_err("unbound symbol");
}

void lenv_put(lenv* e, lval* k, lval* v) {
  // iterate over environment to see if the symbol already exists
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  // create a new entry in the symbol table
  e->count++;
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);

  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);

  e->vals[e->count - 1] = lval_copy(v);
}

lval* lval_abstract() {
  lval* lv = malloc(sizeof(lval));
  lv->type = -1;

  lv->num = 0;
  lv->err = NULL;
  lv->sym = NULL;

  lv->builtin = NULL;
  lv->lenv = NULL;
  lv->formulas = NULL;
  lv->body = NULL;

  lv->count = 0;
  lv->cell = NULL;
  return lv;
}

lval* lval_num(long x) {
  lval* lv = lval_abstract();
  lv->type = LVAL_NUM;
  lv->num = x;

  return lv;
}

lval* lval_err(char* fmt, ...) {
  lval* v = lval_abstract();
  v->type = LVAL_ERR;
  v->err = malloc(512);

  va_list va;
  va_start(va, fmt);

  vsnprintf(v->err, 511, fmt, va);
  v->err = realloc(v->err, strlen(v->err) + 1);
  va_end(va);

  return v;
}

lval* lval_sym(char* s) {
  lval* v = lval_abstract();
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);

  return v;
}

lval* lval_sexpr(void) {
  lval* v = lval_abstract();
  v->type = LVAL_SEXPR;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = lval_abstract();
  v->type = LVAL_QEXPR;
  return v;
}

lval* lval_builtin_function(lbuiltin builtin) {
  lval* v = lval_abstract();
  v->type = LVAL_FUN;
  v->builtin = builtin;
  return v;
}

void lval_del(lval* v) {
  switch(v->type) {
    case LVAL_NUM:
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_FUN:
      break;
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
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } // root
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* lval_copy(lval* v) {
  lval* x = lval_abstract();
  x->type = v->type;

  switch(v->type) {
    case LVAL_FUN:
      x->builtin = v->builtin;
      break;
    case LVAL_NUM:
      x->num = v->num;
      break;
    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err);
      break;
    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym);
      break;
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

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    if (i != (v->count - 1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch(v->type) {
    case LVAL_NUM:
      printf("%li", v->num);
      break;
    case LVAL_ERR:
      printf("Error: %s", v->err);
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_FUN:
      printf("<function>");
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

lval* lval_pop(lval* v, int i) {
  // find the item at "i"
  lval *x = v->cell[i];

  // shift the memory after "i" over it
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  // decrease the count of items in the list
  v->count--;

  // reallocate memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
  // ensure all args are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      int type = a->cell[i]->type;
      lval_del(a);
      return lval_err("Function '%s' passed incorrect type for argument %i. Got %s, expected %s.", op, i, ltype_name(type), ltype_name(LVAL_NUM));
    }
  }

  // pop first element
  lval* x = lval_pop(a, 0);

  // if no args remaining, and op is sub, perform unary negation
  if ((strcmp(op, "-") == 0) && (a->count == 0)) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    // pop next element
    lval* y = lval_pop(a, 0);

    // operation
    if (strcmp(op, "+") == 0) {
      x->num += y->num;
    }
    if (strcmp(op, "-") == 0) {
      x->num -= y->num;
    }
    if (strcmp(op, "*") == 0) {
      x->num *= y->num;
    }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero");
        break;
      }
      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
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

  return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  // error checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  // empty expressions
  if (v->count == 0) {
    return v;
  }

  // single expression
  if (v->count == 1) {
    return lval_take(v, 0);
  }

  // ensure first element is a function after evaluation
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(f);
    lval_del(v);
    return lval_err("first element is not a function");
  }

  // call function
  lval* result = f->builtin(e, v);
  lval_del(f);
  return result;
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_builtin_function(func);

  lenv_put(e, k, v);

  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "cons", builtin_cons);
  lenv_add_builtin(e, "len", builtin_len);
  lenv_add_builtin(e, "init", builtin_init);
  lenv_add_builtin(e, "def", builtin_def);
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

lval* builtin_head(lenv* e, lval* a) {
  if (a->count != 1) {
    return a;
  }
  LASSERT_NUM_ARGS(a, "head", a->count, 1);
  LASSERT_TYPE(a, "head", a->cell[0]->type, LVAL_QEXPR);
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}");

  lval* v = lval_take(a, 0);
  while(v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "tail", a->count, 1);
  LASSERT_TYPE(a, "tail", a->cell[0]->type, LVAL_QEXPR);
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "tail", a->count, 1);
  LASSERT_TYPE(a, "tail", a->cell[0]->type, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE(a, "join", a->cell[0]->type, LVAL_QEXPR);
  }

  lval* x = lval_pop(a, 0);
  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* lval_join(lval* x, lval* y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval* builtin_cons(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "cons", a->count, 2);
  LASSERT_TYPE(a, "cons", a->cell[1]->type, LVAL_QEXPR);

  lval* val = lval_pop(a, 0);
  lval* qexp = lval_pop(a, 0);
  lval_del(a);

  // make room for another value
  qexp->count++;
  realloc(qexp->cell, sizeof(lval*) * qexp->count);

  // memmove to move everything over by one
  memmove(&qexp->cell[1], &qexp->cell[0], sizeof(lval*) * (qexp->count-1));

  // put value into qexpr's children
  qexp->cell[0] = val;

  return qexp;
}

lval* builtin_len(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "len", a->count, 1);
  LASSERT_TYPE(a, "len", a->cell[0]->type, LVAL_QEXPR);

  lval* qexp = a->cell[0];
  int length = qexp->count;

  lval_del(a);

  return lval_num(length);
}

lval* builtin_init(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "init", a->count, 1);
  LASSERT_TYPE(a, "init", a->cell[0]->type, LVAL_QEXPR);

  lval* qexp = lval_pop(a, 0);
  lval_del(a);

  int num_children = qexp->count;
  lval_del(qexp->cell[num_children - 1]);

  qexp->count--;
  realloc(qexp->cell, sizeof(lval*) * qexp->count);

  return qexp;
}

lval* builtin_def(lenv* e, lval* a) {
  LASSERT_TYPE(a, "def", a->cell[0]->type, LVAL_QEXPR);

  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM, "Function 'def' cannot define non-symbol");
  }

  LASSERT(a, syms->count == a->count-1, "Function 'def' cannot define incorrect number of values to symbols");

  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i+1]);
  }

  lval_del(a);
  return lval_sexpr();
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* JoshLisp = mpc_new("joshlisp");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                     \
      number   : /-?[0-9]+/ ;                               \
      symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;           \
      sexpr : '(' <expr>* ')' ;                             \
      qexpr : '{' <expr>* '}' ;                             \
      expr     : <number> | <symbol> | <sexpr> | <qexpr> ;  \
      joshlisp    : /^/ <expr>* /$/ ;                       \
      ", Number, Symbol, Sexpr, Qexpr, Expr, JoshLisp);

  printf("JoshLisp Version 0.0.0.0.3\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  while (1) {
    mpc_result_t r;
    char* input = readline("josh_lisp> ");
    add_history(input);

    if (!input)
      break;

    if (mpc_parse("<stdin>", input, JoshLisp, &r)) {
      mpc_ast_t* ast = r.output;
      lval* x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      mpc_ast_delete(ast);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  lenv_del(e);
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, JoshLisp);
  return 0;
}
