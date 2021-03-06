#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>
#include "mpc.h"
#define LASSERT(args, cond, fmt, ...) \
   if (!(cond)) { \
   lval* err = lval_err(fmt, ##__VA_ARGS__); \
   lval_del(args); \
   return err; \
   }
#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


/* enumeration of possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

/* forward dec. */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);
void lval_print(lval* v);
void lval_print_str(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_eval(lenv* e, lval* v);
lval* lval_copy(lval* v);
lval* lval_err(char* fmt, ...);
lval* builtin(lval* a, char* builtin);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* lval_join(lval* x, lval* y);
void lval_del(lval* v);

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

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN:   return "Function";
    case LVAL_NUM:   return "Number";
    case LVAL_ERR:   return "Error";
    case LVAL_SYM:   return "Symbol";
    case LVAL_STR:   return "String";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default:         return "Unknown";
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

lval* lval_str(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_STR;
  v->str = malloc(strlen(s) + 1);
  strcpy(v->str, s);
  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;

  /* Set Builtin to Null */
  v->builtin = NULL;

  /* Build new environment */
  v->env = lenv_new();

  /* Set formals and body */
  v->formals = formals;
  v->body = body;
  return v;
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
  /* Iterate over all items in environment */
  for (int i = 0; i < e->count; i++) {
    /* Check if the stored string matches the symbol string */
    /* If it does, return a copy of the value */
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  /* If no symbol found, check parent or error if parent NULL */
  if (e->par) {
    return lenv_get(e->par, k);
  }
  else {
    return lval_err("Unbound symbol! %s", k->sym);
  }
}

void lenv_put(lenv* e, lval* k, lval* v) {
  /* Iterate over all items in environment */
  for (int i = 0; i < e->count; i++) {
  /* If found, delete at that position and replace with new value*/
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  /* If no existing entry, allocate space for a new entry */
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  /* Copy contents of lval and symbol string into new location */
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));
  n->par    = e->par;
  n->count  = e->count;
  n->syms   = malloc(sizeof(char*) * n->count);
  n->vals   = malloc(sizeof(lval*) * n->count);
  for(int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }
  return n;
}

void lenv_def(lenv* e, lval* k, lval* v) {
  /* To define something global, 
     iterate to the top environment then assign */
  while (e->par) { e = e->par; }
  lenv_put(e, k, v);
}

lval* lval_fun(lbuiltin builtin) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = builtin;
  return v;
}

/* pointer to new number type lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* pointer to new error type lval */
lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  /* create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* allocate 512 bytes of space */
  v->err = malloc(512);

  /* printf into the error string w/ a max of 511 chars */
  vsnprintf(v->err, 511, fmt, va);

  /* realloc to number of bytes actually needed */
  v->err = realloc(v->err, strlen(v->err)+1);

  /* clean up our va list */
  va_end(va);

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

/* pointer to new Qexpr type lval */
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

/* delete an lval* */
void lval_del(lval* v) {

  switch (v->type) {

  /* String */
  case LVAL_STR: free(v->str); break;

  /* Do nothing special for number or function types */
  case LVAL_NUM: break;
  case LVAL_FUN: 
    if (!v->builtin) {
      lenv_del(v->env);
      lval_del(v->formals);
      lval_del(v->body);
    }
  break;

  /* For Err or Sym free the string data */
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;
  
  /* If Qexpr or Sexpr then delete all elements inside */
  case LVAL_QEXPR:
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

lval* lval_read_str(mpc_ast_t* t) {
  /* Cut off the final quote character */
  t->contents[strlen(t->contents)-1] = '\0';
  /* Copy the string missing out the first quote character */
  char* unescaped = malloc(strlen(t->contents+1)+1);
  strcpy(unescaped, t->contents+1);
  /* Pass through the unescape function */
  unescaped = mpcf_unescape(unescaped);
  /* Construct a new lvla using the string */
  lval* str= lval_str(unescaped);
  /* Free the mallocs! */
  free(unescaped);
  return str;
}

lval* lval_read(mpc_ast_t* t) {
  /* If Symbol, Number, or String return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  if (strstr(t->tag, "string")) { return lval_read_str(t); }

  /* If root (>) or sexpr then create empty list*/
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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
    case LVAL_FUN: 
      if (v->builtin) {
        printf("<function>"); 
      } else {
        printf("(\\ "); 
        lval_print(v->formals); 
        putchar(' '); 
        lval_print(v->body);
        putchar(')'); 
      }
    break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_STR:   lval_print_str(v); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}

void lval_print_str(lval* v) {
  /* Make a copy of the string */
  char* escaped = malloc(strlen(v->str) + 1);
  strcpy(escaped, v->str);
  /* Pass it through escape function */
  escaped = mpcf_escape(escaped);
  /* Print it between quote characters */
  printf("\"%s\"", escaped);
  /* clean up after ourselves */
  free(escaped);
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

lval* lval_copy(lval* v) {

  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch(v->type) {
    /* String copy your string */
    case LVAL_STR: x->str = malloc(strlen(v->str) + 1);
      strcpy(x->str, v->str); break;

    /* Copy functions and numbers directly */
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
    case LVAL_NUM: x->num = v->num; break;

    /* Copy Strings using malloc and strcpy */
    case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); 
      strcpy(x->err, v->err);
      break;
    case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym);
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

lval* lval_call(lenv* e, lval* f, lval* a) {
  /* If builtin, simply apply it*/
  if (f->builtin) { return f->builtin(e,a); }

  /* record argumnet counts */
  int given = a->count;
  int total = f->formals->count;

  /* while items still remain to be processes */
  while (a->count) {
    /* If we've run out of formal arguments to bind */
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err("Function passed to many arguments.  Given %i, Expected %i.", given, total);
    }
    /* Pop the first symbol from the formals */
    lval* sym = lval_pop(f->formals, 0);

    /* Special case to deal with the '&' character. */
    if (strcmp(sym->sym, "&") == 0) {
      /* Ensure '&' is followed by another symbol */
      if(f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid.  Symbol '&' not followed by a single symbol.");
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

    /* Delete the symbol and value */
    lval_del(sym);
    lval_del(val);
  }

  /* argument list now bound, so can be cleaned up */
  lval_del(a);
    
  /* If '%' remains in formal list, it should be bound to empty list */
  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
    /* check to insure that '&' is not passed invalidly */
      if (f->formals->count != 2) {
        return lval_err("Function format invalid.  Symbol '&' not followed by a single symbol.");
      }
      /* Pop & delete the symbol */
      lval_del(lval_pop(f->formals, 0));

      /* Pop the next symbol and create an empty list */
      lval* sym = lval_pop(f->formals, 0);
      lval* val = lval_qexpr();

      /* Bind to the env and delete */
      lenv_put(f->env, sym, val);
      lval_del(sym);
      lval_del(val);
    }

  /* If all the formals have been found, evaluate */
  if (f->formals->count == 0) {
    /* set funciton environment to parent to current */ 
    f->env->par = e;
    /* evaluate and return */
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } 
  else {
    /* otherwise, return a partially evaluated function */
    return lval_copy(f);
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

lval* lval_eval_sexpr(lenv* e, lval* v) {

  /* Evaluate children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  /* Error checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* Empty expression */
  if (v->count == 0) { return v;}

  /* Single expression */
  if (v->count == 1) { return lval_take(v, 0);}

  /* Ensure First element is a function after evaluation */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(f); lval_del(v);
    return lval_err("first element is not a function");
  }

  /* If so call function to get result */
  //lval* result = f->builtin(e, v);
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

/* Let us eval */
lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e,v);
    lval_del(v);
    return x;
  }
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  /* All other lval types remain the same */
  return v;
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift the memory following the item at "i" over the top of it */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /*Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_lambda(lenv* e, lval* a) {
  /* check two arguments */
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  /* check first QEXPR only contains symbols */
  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
      "Can not define non-symbol.  Got %s, expected %s.",
      ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }
  
  /* Pop first two arguments and pass them to lval_lambda */
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments! Expected %i, got %i", a->count, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'head' passed incorrect types.  Got %s, expected %s", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR) );
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!"); 

  /* Otherwise take the first argument */
  lval* v = lval_take(a, 0);
  /* Delete all elements that are not head and return */
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1), "Function 'tail' passed too many arguments!");
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'tail' passed incorrect types!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!"); 

  /* Take the first argument */
  lval* v = lval_take(a, 0);

  /* Delete first element and return */
   lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1), "Function 'eval' passed too many arguments!");
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect types!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, (a->cell[i]->type == LVAL_QEXPR), "Function 'join' passed incorrect types!");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
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

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

lval* builtin_op(lenv* e, lval* a, char* op) {
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if(a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on a non-number!");
    }
  }
  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no argument and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

  /* While there are still elements remaining */
  while (a->count > 0) {
    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    /* perform the next operation */
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by Zero!");
        break;
      }
      x->num /= y->num;
    }
    /* Delete element no that we're finished with it */
    lval_del(y);
  }
  /* delete input expression and return result */
  lval_del(a);
  return x;
}

lval* builtin_var(lenv* e, lval* a, char* func ) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

  /* First argument is a symbol list */ 
  lval* syms = a->cell[0];

  /* Ensure all elements of first list are symbols */
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM), "Function 'def' can not define a non-symbol!");
  }

  /* Check correct number of symbols and values */
  LASSERT(a, (syms->count == a->count-1), "Function 'def' can not define incorrect incorrect number of values to symbols");

  /* If 'def' define in global scope, 'put' in local scope. */
  for (int i = 0; i < syms->count; i++) {
    if (strcmp(func, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); } 
    if (strcmp(func, "=") == 0)   { lenv_put(e, syms->cell[i], a->cell[i+1]); } 
  }

  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "="); }

void lenv_add_builtin(lenv* e, char* name, lbuiltin builtin) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(builtin);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

lval* builtin_ord(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  LASSERT_TYPE(op, a, 0, LVAL_NUM);
  LASSERT_TYPE(op, a, 1, LVAL_NUM);

  int r;
  if (strcmp(op, ">")  == 0) { r = (a->cell[0]->num >  a->cell[1]->num); }
  if (strcmp(op, "<")  == 0) { r = (a->cell[0]->num <  a->cell[1]->num); }
  if (strcmp(op, ">=") == 0) { r = (a->cell[0]->num >= a->cell[1]->num); }
  if (strcmp(op, "<=") == 0) { r = (a->cell[0]->num <= a->cell[1]->num); }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_gt(lenv* e, lval* a) { return builtin_ord(e, a, ">");  }
lval* builtin_lt(lenv* e, lval* a) { return builtin_ord(e, a, "<");  }
lval* builtin_ge(lenv* e, lval* a) { return builtin_ord(e, a, ">=");  }
lval* builtin_le(lenv* e, lval* a) { return builtin_ord(e, a, "<=");  }

int lval_eq(lval* x, lval* y) {
  /* Different types are always unequal */
  if (x->type != y->type) { return 0; }

  /* Compare based on type */
  switch(x->type) {
    /* Compare our string */
    case LVAL_STR: return (strcmp(x->str, y->str) == 0);
    /* Compare number*/
    case LVAL_NUM: return (x->num == y->num);
    /* Compare Error/Symbol */
    case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
    case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);

    /* If Builtin, compare functions.  Otherwise compare formals and body */
    case LVAL_FUN:
      if (x->builtin || y->builtin) {
        return x->builtin == y->builtin;
      } else {
        return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body); 
      }
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      if (x->count != y->count) { return 0; }
      for (int i =0; i < x->count; i++) {
        /* If any any element not equal, then the whole list isn't equal */
        if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
      }
      /* otherwise, return 1 */
      return 1;
    break;
  }
 return 0;
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  int r;
  if (strcmp(op, "==") == 0) { r =  lval_eq(a->cell[0], a->cell[1]); } 
  if (strcmp(op, "!=") == 0) { r = !lval_eq(a->cell[0], a->cell[1]); }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a) { return builtin_cmp(e, a, "==");}
lval* builtin_ne(lenv* e, lval* a) { return builtin_cmp(e, a, "!=");}

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_NUM("if", a, 3);
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);
  
  /* mark both expressions as evaluable */
  lval* x;
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  if (a->cell[0]->num) {
    /* if condition is true, evaluate first expression */
    x = lval_eval(e, lval_pop(a, 1));
  } else {
    x = lval_eval(e, lval_pop(a, 2));
  }
  /* delte arglist and return */
  lval_del(a);
  return x;
}

void lenv_add_builtins(lenv* e) {
  /* lambda the mighty */
  lenv_add_builtin(e, "\\",  builtin_lambda); 
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  /* Variable Functions */
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=",   builtin_put);
  /* Mathematical Functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  /* Comparison Functions */
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_ne);
  lenv_add_builtin(e, ">", builtin_gt);
  lenv_add_builtin(e, "<", builtin_lt);
  lenv_add_builtin(e, ">=", builtin_ge);
  lenv_add_builtin(e, "<=", builtin_le);
}

int main(int argc, char** argv) {

  /* Create some parsers */
  mpc_parser_t* String    = mpc_new("string");
  mpc_parser_t* Number    = mpc_new("number");
  mpc_parser_t* Symbol    = mpc_new("symbol");
  mpc_parser_t* Sexpr     = mpc_new("sexpr");
  mpc_parser_t* Qexpr     = mpc_new("qexpr");
  mpc_parser_t* Expr      = mpc_new("expr");
  mpc_parser_t* Lithp     = mpc_new("lithp");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, 
    "\
    string : /\"(\\\\.|[^\"])*\"/; \
    number : /-?[0-9]+/; \
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
    sexpr  : '(' <expr>* ')'; \
    qexpr  : '{' <expr>* '}'; \
    expr   : <number> | <symbol> | <sexpr> | <qexpr> | <string>; \
    lithp  : /^/ <expr>* /$/; \
    ",
  String, Number, Symbol, Sexpr, Qexpr, Expr, Lithp);

  /* Print version and exit */
  puts("Lithp Version 0.0.0.0.1");
  puts("Suitable for absolutely nothing!");
  puts("Press Ctrl-C to exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

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
        /* Get a real result and print it */
        lval* x = lval_eval(e, lval_read(r.output));
        lval_println(x);
        lval_del(x);

        mpc_ast_delete(r.output);
      } else {
        /* Otherwise, print the error */
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
      }
      /* free the malocs! */
      free(input);
  }
  /* Clean up our environment */
  lenv_del(e);
  /* Undefine and Delete our Parsers.  Or else... */
  mpc_cleanup(6, String, Number, Symbol, Sexpr, Qexpr, Expr, Lithp);
  return 0;
}

