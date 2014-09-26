#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#include "strings.h"

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_BOOL: return "Boolean";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_STR: return "String";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));

  e->par = NULL;
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

  if (e->par) {
    return lenv_get(e->par, k);
  } else {
    return lval_err("unbound symbol '%s'", k->sym);
  }
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

void lenv_def(lenv* e, lval* k, lval* v) { // global variables
  while(e->par) {
    e = e->par;
  }

  lenv_put(e, k, v);
}

lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));

  n->par = e->par;
  n->count = e->count;

  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);

  for (int i = 0; i < n->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);

    n->vals[i] = lval_copy(e->vals[i]);
  }

  return n;
}

lval* lval_abstract() {
  lval* lv = malloc(sizeof(lval));
  lv->type = -1;

  lv->num = 0;
  lv->err = NULL;
  lv->sym = NULL;
  lv->str = NULL;

  lv->builtin = NULL;
  lv->env = NULL;
  lv->formals = NULL;
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

lval* lval_bool(int x) {
  lval* v = lval_abstract();
  v->type = LVAL_BOOL;
  v->num = (x != 0); // x is 1 or 0
  return v;
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

lval* lval_str(char* s) {
  lval* v = lval_abstract();
  v->type = LVAL_STR;
  v->str = malloc(strlen(s) + 1);
  strcpy(v->str, s);

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

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = lval_abstract();
  v->type = LVAL_FUN;

  v->env = lenv_new(); // build new env

  v->formals = formals;
  v->body = body; // set formals and body

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
    case LVAL_STR:
      free(v->str);
      break;
    case LVAL_FUN:
      if (!v->builtin) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
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

lval* lval_read_str(mpc_ast_t* t) {
  t->contents[strlen(t->contents) - 1] = '\0'; // cut off final " character
  char* unescaped = malloc(strlen(t->contents + 1) + 1); // everything but the first " char
  strcpy(unescaped, t->contents+1);

  unescaped = mpcf_unescape(unescaped);
  lval* str = lval_str(unescaped);
  free(unescaped);

  return str;
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }
  if (strstr(t->tag, "string")) {
    return lval_read_str(t);
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
    if (strstr(t->children[i]->tag, "comment")) { continue; }

    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* lval_copy(lval* v) {
  lval* x = lval_abstract();
  x->type = v->type;

  switch(v->type) {
    case LVAL_FUN:
      if (v->builtin) {
        x->builtin = v->builtin;
      } else {
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
      break;
    case LVAL_BOOL:
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
    case LVAL_STR:
      x->str = malloc(strlen(v->str) + 1);
      strcpy(x->str, v->str);
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

void lval_print_str(lval* v) {
  char* escaped = malloc(strlen(v->str) + 1);
  strcpy(escaped, v->str);

  escaped = mpcf_escape(escaped);
  printf("\"%s\"", escaped);

  free(escaped);
}

void lval_print(lval* v) {
  switch(v->type) {
    case LVAL_NUM:
      printf("%li", v->num);
      break;
    case LVAL_BOOL:
      printf(v->num ? "true" : "false");
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
    case LVAL_FUN:
      if (v->builtin) {
        printf("<builtin function>");
      } else {
        printf("(\\ ");
        lval_print(v->formals);
        putchar(' ');
        lval_print(v->body);
        putchar(')');
      }
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
    lval* err = lval_err("s-expression starts with wrong type. Got %s, expected %s.", ltype_name(f->type), ltype_name(LVAL_FUN));
    lval_del(f);
    lval_del(v);

    return err;
  }

  // call function
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  // if builtin, just call that
  if (f->builtin) {
    return f->builtin(e, a);
  }

  // record arg counts
  int given = a->count;
  int total = f->formals->count;

  while(a->count) {
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed too many arguments. Got %i, expected %i.", given, total);
    }

    lval* sym = lval_pop(f->formals, 0);

    if (strcmp(sym->sym, "&") == 0) {
      // ensure '&' is followed by another symbol
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
      }

      // next formal bound to remaining args
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }

    lval* val = lval_pop(a, 0);

    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  lval_del(a);

  // if '&' remains, it should be bound to empty list
  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
    }

    lval_del(lval_pop(f->formals, 0)); // delete '&'
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();

    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  if (f->formals->count == 0) {
    f->env->par = e;
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    return lval_copy(f);
  }
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_builtin_function(func);

  lenv_put(e, k, v);

  lval_del(k);
  lval_del(v);
}

void lenv_add_builtin_constant(lenv* e, char* name, lval* val) {
  lval* k = lval_sym(name);
  lenv_put(e, k, val);

  lval_del(k);
  lval_del(val);
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
  lenv_add_builtin(e, "=", builtin_put);
  lenv_add_builtin(e, "\\", builtin_lambda);
  lenv_add_builtin(e, ">", builtin_gt);
  lenv_add_builtin(e, ">=", builtin_gte);
  lenv_add_builtin(e, "<", builtin_lt);
  lenv_add_builtin(e, "<=", builtin_lte);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_ne);
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin_constant(e, "true", lval_bool(1));
  lenv_add_builtin_constant(e, "false", lval_bool(0));
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
  LASSERT_NUM_ARGS(a, "eval", a->count, 1);
  LASSERT_TYPE(a, "eval", a->cell[0]->type, LVAL_QEXPR);

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
  qexp->cell = realloc(qexp->cell, sizeof(lval*) * qexp->count);

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
  qexp->cell = realloc(qexp->cell, sizeof(lval*) * qexp->count);

  return qexp;
}

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE(a, func, a->cell[0]->type, LVAL_QEXPR);

  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    if (syms->cell[i]->type != LVAL_SYM) {
      lval_del(a);
      return lval_err("Function '%s' cannot define non-symbol", func);
    }
  }

  if (syms->count != a->count-1) {
    lval_del(a);
    return lval_err("Function '%s' cannot define incorrect number of values to symbols", func);
  }

  for (int i = 0; i < syms->count; i++) {
    if (strcmp("=", func) == 0) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    if (strcmp("def", func) == 0) {
      lenv_def(e, syms->cell[i], a->cell[i+1]);
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
  LASSERT_NUM_ARGS(a, "\\", a->count, 2);
  LASSERT_TYPE(a, "\\", a->cell[0]->type, LVAL_QEXPR);
  LASSERT_TYPE(a, "\\", a->cell[1]->type, LVAL_QEXPR);

  // check that first qexp contains only symbols
  for (int i = 0; i < a->count; i++) {
    if (a->cell[0]->cell[i]->type != LVAL_SYM) {
      lval_del(a);
      return lval_err("Cannot define non-symbol. Got %s, expected %s", ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }
  }

  // pop first two args and pass them to lval_lambda
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_ord(lenv* e, lval* a, char* op) {
  LASSERT_NUM_ARGS(a, op, a->count, 2);
  LASSERT_TYPE(a, op, a->cell[0]->type, LVAL_NUM);
  LASSERT_TYPE(a, op, a->cell[1]->type, LVAL_NUM);

  int r;

  if (strcmp(op, ">") == 0) {
    r = (a->cell[0]->num > a->cell[1]->num);
  }
  if (strcmp(op, ">=") == 0) {
    r = (a->cell[0]->num >= a->cell[1]->num);
  }
  if (strcmp(op, "<") == 0) {
    r = (a->cell[0]->num < a->cell[1]->num);
  }
  if (strcmp(op, "<=") == 0) {
    r = (a->cell[0]->num <= a->cell[1]->num);
  }

  lval_del(a);
  return lval_bool(r);
}

lval* builtin_gt(lenv* e, lval* a) {
  return builtin_ord(e, a, ">");
}

lval* builtin_gte(lenv* e, lval* a) {
  return builtin_ord(e, a, ">=");
}

lval* builtin_lt(lenv* e, lval* a) {
  return builtin_ord(e, a, "<");
}

lval* builtin_lte(lenv* e, lval* a) {
  return builtin_ord(e, a, "<=");
}

int lval_eq(lval* a, lval* b) {
  if (a->type != b->type) {
    return 0;
  }

  switch (a->type) {
    case LVAL_NUM:
      return a->num == b->num;
    case LVAL_ERR:
      return !strcmp(a->err, b->err);
    case LVAL_SYM:
      return !strcmp(a->sym, b->sym);
    case LVAL_STR:
      return !strcmp(a->str, b->str);
    case LVAL_FUN:
      if (a->builtin) {
        return a->builtin == b->builtin;
      } else {
        return lval_eq(a->formals, b->formals) && lval_eq(a->body, b->body);
      }
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      if (a->count != b->count) {
        return 0;
      }

      for (int i = 0; i < a->count; i++) {
        if (!lval_eq(a->cell[i], b->cell[i])) {
          return 0;
        }
      }

      return 1;
      break;
  }

  return 0;
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
  LASSERT_NUM_ARGS(a, op, a->count, 2);

  lval* x = lval_pop(a, 0);
  lval* y = lval_pop(a, 0);

  int result = 0;
  if (strcmp(op, "==") == 0) {
    result = lval_eq(x, y);
  } else if (strcmp(op, "!=") == 0) {
    result = !lval_eq(x, y);
  }

  lval_del(a);

  return lval_bool(result);
}

lval* builtin_eq(lenv* e, lval* a) {
  return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a) {
  return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_NUM_ARGS(a, "if", a->count, 3);
  LASSERT_TYPE(a, "if", a->cell[0]->type, LVAL_BOOL);
  LASSERT_TYPE(a, "if", a->cell[1]->type, LVAL_QEXPR);
  LASSERT_TYPE(a, "if", a->cell[2]->type, LVAL_QEXPR);

  lval* condition = lval_pop(a, 0);
  lval* then_function = lval_pop(a, 0);
  lval* else_function = lval_pop(a, 0);

  lval* result;
  if (condition->num) {
    then_function->type = LVAL_SEXPR;
    result = lval_eval(e, then_function);
  } else {
    else_function->type = LVAL_SEXPR;
    result = lval_eval(e, else_function);
  }

  lval_del(a);

  return result;
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* String = mpc_new("string");
  mpc_parser_t* Comment = mpc_new("comment");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* JoshLisp = mpc_new("joshlisp");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                     \
      number   : /-?[0-9]+/ ;                               \
      symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;           \
      string : /\"(\\\\.|[^\"])*\"/ ;                       \
      comment : /;[^\\r\\n]*/ ;                             \
      sexpr : '(' <expr>* ')' ;                             \
      qexpr : '{' <expr>* '}' ;                             \
      expr     : <number> | <symbol> | <string>             \
               | <sexpr> | <qexpr> | <comment> ;  \
      joshlisp    : /^/ <expr>* /$/ ;                       \
      ", Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, JoshLisp);

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
  mpc_cleanup(6, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, JoshLisp);
  return 0;
}
