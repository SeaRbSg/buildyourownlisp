struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);
struct lval {
  int type;

  long num;
  char* err;
  char* sym;
  lbuiltin fun;

  int count; // count and pointer to list of lval*
  struct lval** cell;
};

struct lenv {
  int count;
  char** syms;
  lval** vals;
};

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR }; // lval_types

char* ltype_name(int t);
lenv* lenv_new(void);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
lval* lval_abstract();
lval* lval_num(long x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* x);
lval* lval_sexpr(void);
lval* lval_qepxr(void);
lval* lval_fun(lbuiltin func);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_copy(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* y);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* lval_join(lval* x, lval* y);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
lval* builtin_def(lenv* e, lval* a);


#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    lval * err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }
