typedef struct lval {
  int type;
  long num;

  char* err;
  char* sym;

  int count; // count and pointer to list of lval*
  struct lval** cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR }; // lval_types

lval* lval_abstract();
lval* lval_num(long x);
lval* lval_err(char* m);
lval* lval_sym(char* x);
lval* lval_sexpr(void);
lval* lval_qepxr(void);
void lval_del(lval* v);
lval* lval_add(lval* v, lval* x);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* v);
void lval_println(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* builtin_op(lval* a, char* op);
lval* lval_eval(lval* v);
lval* lval_eval_sexpr(lval* y);
lval* builtin(lval* a, char* func);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* builtin_cons(lval* a);
lval* lval_join(lval* x, lval* y);
lval* builtin_len(lval* a);

#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }
