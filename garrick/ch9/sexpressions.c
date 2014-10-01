#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"

/* enumeration of possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

/* enumeration of possible error types */
/* enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM }; */

/* type, type baby */
typedef struct lval {
  int type;

  long num;

  char* err;
  char* sym;

  /*Count & pointer to list of 'lval' */
  int count;
  struct lval** cell;


} lval;

/* forward dec. */
void lval_print(lval* v);

/* pointer to new number type lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* pointer to new error type lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* pointer to new symbol type lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym  = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* pointer to new sexpr type lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* delete an lvla* */
void lval_del(lval* v) {

  switch (v->type) {

  /* Do nothing special for number type */
  case LVAL_NUM: break;

  /* For Err or Sym free the string data */
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;
  
  /* If Sexpr then delete all elements inside */
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);    
    }
    /* Also free the memory allocated to contain the pointers */
    free(v->cell);
  break;  
  }
  /*  Don't forget to free the memory allocated for the lval struct itself! */
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
  /* If Symbol or Number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list*/
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr") == 0) { x = lval_sexpr(); }

  /* Fill this list with any valid expression contained within */
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
    /* */
    lval_print(v->cell[i]);

    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}


/* Print an lval */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}



/* print and lval followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* our_exponent(lval* base, lval* exp) {
  if(exp->num <= 1) {
    return base;
    }
  else {
    return lval_num(base->num * our_exponent(base, lval_num(exp->num - 1))->num);
  }
}

lval* min(lval* x, lval* y) {
  if(x->num <= y->num) {
    return x;
    } 
  return y;
}

lval* max(lval* x, lval* y) {
  if(x->num >= y->num) {
    return x;
    } 
  return y;
}

/* Use operator string to see which operator to perform */
/*
lval eval_op(lval x, char* op, lval y) {
  * bail out on errors*
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  * math up the numbers *
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { 
      return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); 
    }
  if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
  if (strcmp(op, "^") == 0) { return our_exponent( x, y); }
  if (strcmp(op, "min") == 0) { return min( x, y); }
  if (strcmp(op, "max") == 0) { return max( x, y); }

  * wtf lol idk *
  return lval_err(LERR_BAD_OP);
}
*/

/* Let us eval */
/*
lval eval(mpc_ast_t* t) {

  if (strstr(t->tag, "number")) {
    * check for an error in conversion *
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM); 
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while(strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}
*/

int main(int argc, char** argv) {

  /* Create some parsers */
  mpc_parser_t* Number    = mpc_new("number");
  mpc_parser_t* Symbol    = mpc_new("symbol");
  mpc_parser_t* Sexpr     = mpc_new("sexpr");
  mpc_parser_t* Expr      = mpc_new("expr");
  mpc_parser_t* Lithp     = mpc_new("lithp");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, 
    "\
    number : /-?[0-9]+/; \
    symbol : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\"; \
    sexpr  : '(' <expr>* ')'; \
    expr   : <number> | <symbol> | <sexpr>; \
    lithp  : /^/ <expr>* /$/; \
    ",
  Number, Symbol, Sexpr, Expr, Lithp);

  /* Print version and exit */
  puts("Lithp Version 0.0.0.0.1");
  puts("Suitable for absolutely nothing!");
  puts("Press Ctrl-C to exit\n");

  /* never ending storrr-y! */
  while(1) {
      /* output our prompt and get input */ 
      char* input = readline("lithp> ");
      if (!input) break;
      /* add to our history */
      add_history(input);
      /* Attempt to parse the user input */
      mpc_result_t r;
      if (mpc_parse("<stdin>", input, Lithp, &r)) {
        /* On success, print the AST */
        //mpc_ast_print(r.output);
        /* Get a real result and print it */
        lval* result = lval_read(r.output);
        lval_println(result);
        lval_del(r.output);
      } else {
        /* Otherwise, print the error */
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
      }
      /* free the malocs! */
      free(input);
  }
  /* Undefine and Delete our Parsers.  Or else... */
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lithp);
  return 0;
}

