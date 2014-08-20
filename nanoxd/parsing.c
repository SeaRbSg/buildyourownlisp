#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

// Fake add_history command
void add_history(char* unused) {}

// Otherwise include the editline headers
#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

// Create Enumeration of possible lval types
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

// Declare new lval struct
typedef struct lval {
  int type;
  long num;
  char* err;
  char* sym;

  // Counter and pointer to a list of lval
  int count;
  struct lval** cell;

} lval;

// Enumerable for possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Forward declare variables
void lval_print(lval* v);

// Create a new number type lval pointer
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
    // Do nothing special for number type
    case LVAL_NUM: break;

    //For Err or Sym free the string data
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    //If Sexpr then delete all elements inside
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      // Also free the memory allocated to contain the pointers
      free(v->cell);
    break;
  }

  // Finally free the memory allocated for the lval struct itself
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_pop(lval* v, int i) {
  // Find item at [i]
  lval* x = v->cell[i];

  // Shift the memory following the item at i over the top of it
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  // Decrease the count of items in the list
  v->count--;

  // Reallocate memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) :lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
  // If Symbol or Number return conversion to taht type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  // If root (>) or sexpr then create empty list
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strcmp(t->tag, "sexpr"))  { x = lval_sexpr(); }

  // Fill list with any valid expression contained within
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
    // Print value contained within
    lval_print(v->cell[i]);

    // Don't print trailing space if last element
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

// Print an "lval"
void lval_print(lval* v) {
  switch (v->type) {
    // If type is a number print it, then break out of the switch
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

// Print an lval followed by a newline
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* builtin_op(lval* a, char* op) {
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non number!");
    }
  }

  // Pop the first element
  lval* x = lval_pop(a, 0);

  // If no arguments and sub then perform unary negation
  if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

  // While there are still elements running
  while(a->count > 0) {
    // Pop the next element
    lval* y = lval_pop(a, 0);

    // Perform operation
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division by Zero!"); break;
      } else {
        x->num /= y->num;
      }
    }

     // Delete finished element
     lval_del(y);
  }

  // Delete Input expression and return result
  lval_del(a);
  return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {
  // Evaluate children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // Error checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Empty expression
  if (v->count == 0) { return v; }

  // Single Expression
  if (v->count == 1) { return lval_take(v, 0); }

  // Ensure first element is a symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression does not start with symbol!");
  }

  // Call builtin with operator
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  // Evaluate sexpr
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

  // All other lval types remain the same
  return v;
}

int main(int argc, char** argv) {

  // Create parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");

  mpca_lang(MPC_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      symbol   : '+' | '-' | '*' | '/' | '%' | '^' ;      \
      sexpr    : '(' <expr>* ')' ;                        \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Symbol, Sexpr, Expr, Lispy);

  // Print version and exit information
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+C to exit\n");

  // In a never ending loop
  while (1) {
    // Output prompt and get input
    char* input = readline("darkLisp> ");

    // Add input to history
    add_history(input);

    // Attempt to parse the user input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      // Print the error
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    // Free retrieved input
    free(input);
  }

  // Undefine and delete our parsers
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

  return 0;
}
