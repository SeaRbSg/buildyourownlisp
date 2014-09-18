#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#include "q_expressions.h"

lval* lval_abstract() {
  lval* lv = malloc(sizeof(lval));
  lv->type = -1;
  lv->num = 0;
  lv->err = NULL;
  lv->sym = NULL;

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

lval* lval_err(char* m) {
  lval* lv = lval_abstract();
  lv->type = LVAL_ERR;
  lv->err = malloc(strlen(m) + 1);
  strcpy(lv->err, m);

  return lv;
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

lval* builtin_op(lval* a, char* op) {
  // ensure all args are numbers
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number");
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

lval* lval_eval(lval* v) {
  // evaluate Sexpressions
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  // all other lval types stay the same
  return v;
}

lval* lval_eval_sexpr(lval* v) {
  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
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

  // ensure first element is a symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression does not start with symbol");
  }

  // call builtin
  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
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
  if (strstr("+-*/", func)) {
    return builtin_op(a, func);
  }

  lval_del(a);
  return lval_err("Unknown function");
}

lval* builtin_head(lval* a) {
  if (a->count != 1) {
    return a;
  }
  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed wrong type");
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}");

  lval* v = lval_take(a, 0);
  while(v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lval* a) {
  LASSERT(a, a->count == 1, "Function 'tail' passed too many args");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed wrong type");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed too many args");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_join(lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type");
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

lval* builtin_cons(lval* a) {
  LASSERT(a, a->count == 2, "Function 'cons' must be passed two arguments");
  LASSERT(a, a->cell[1]->type == LVAL_QEXPR, "Second argument must be a Q-expression");

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

lval* builtin_len(lval* a) {
  LASSERT(a, a->count == 1, "Function 'len' must be passed one argument");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'len' passed wrong type");

  lval* qexp = a->cell[0];
  int length = qexp->count;

  lval_del(a);

  return lval_num(length);
}

lval* builtin_init(lval* a) {
  LASSERT(a, a->count == 1, "Function 'len' must be passed one argument");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'len' passed wrong type");

  lval* qexp = lval_pop(a, 0);
  lval_del(a);

  int num_children = qexp->count;
  lval_del(qexp->cell[num_children - 1]);

  qexp->count--;
  realloc(qexp->cell, sizeof(lval*) * qexp->count);

  return qexp;
}

int main(int argc, char** argv) {

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* JoshLisp = mpc_new("joshlisp");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                   \
      number   : /-?[0-9]+/ ;                             \
      symbol : \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" | \"len\" | \"init\" | '+' | '-' | '*' | '/' ;                    \
      sexpr : '(' <expr>* ')' ;                           \
      qexpr : '{' <expr>* '}' ;                           \
      expr     : <number> | <symbol> | <sexpr> | <qexpr> ;\
      joshlisp    : /^/ <expr>* /$/ ;             \
      ", Number, Symbol, Sexpr, Qexpr, Expr, JoshLisp);

  printf("JoshLisp Version 0.0.0.0.3\n");

  while (1) {
    mpc_result_t r;
    char* input = readline("josh_lisp> ");
    add_history(input);

    if (!input)
      break;

    if (mpc_parse("<stdin>", input, JoshLisp, &r)) {
      mpc_ast_t* ast = r.output;
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      mpc_ast_delete(ast);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, JoshLisp);
  return 0;
}
