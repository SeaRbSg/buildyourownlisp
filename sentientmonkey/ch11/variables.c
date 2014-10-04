#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"

#define ERR_BUF_SIZE 512

#define STR_EQ(A,B)   (strcmp(A,B) == 0)
#define MIN(X,Y)    ((X < Y) ? X : Y)
#define MAX(X,Y)    ((X > Y) ? X : Y)
#define LCHECK(args, cond, fmt, ...) \
    if (!(cond)) {\
        lval* err = lval_err(fmt, ##__VA_ARGS__);\
        lval_del(args);\
        return err;\
    }

#define LCHECK_TYPE(func, arg, t) \
   LCHECK(arg, (arg->type == t), \
       "Function '%s' passed incorrect type. Got %s, Expected %s", \
       func, ltype_name(arg->type), ltype_name(t));

#define LCHECK_COUNT(func, arg, n) \
   LCHECK(arg, (arg->count == n), \
       "Function '%s' passed incorrect number of arguments! Got %i, Expected %i.", \
       func, a->count, n);

#define LCHECK_EMPTY(func, arg) \
    LCHECK(arg, (arg->count != 0), \
        "Function '%s' passed {}!", \
        func);

/* foward declarations */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* creating enums without typedef feels wrong, so I added them. */
typedef enum { LVAL_ERR, LVAL_NUM, LVAL_DUB, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR } lval_type_t;

char* ltype_name(lval_type_t t) {
    switch(t) {
        case LVAL_FUN:
            return "Function";
        case LVAL_NUM:
            return "Number";
        case LVAL_ERR:
            return "Error";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        default:
            return "Unknown";
    }
}

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    lval_type_t type;
    long num;
    double dub;
    char* err;
    char* sym;
    lbuiltin fun;

    int count;
    lval** cell;
};

lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_dub(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_DUB;
    v->dub = x;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->err = malloc(ERR_BUF_SIZE);
    vsnprintf(v->err, ERR_BUF_SIZE-1, fmt, va);

    v->err = realloc(v->err, strlen(v->err)+1);

    va_end(va);

    return v;
}

lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
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

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
        case LVAL_DUB:
        case LVAL_FUN:
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            /* free all elements */
            for (int i=0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* fee memory for pointers */
            free(v->cell);
            break;
    }
    /* free lval struct */
    free(v);
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_FUN:
            x-> fun = v->fun;
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;
        case LVAL_DUB:
            x->dub = v->dub;
            break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err + 1));
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym + 1));
            strcpy(x->sym, v->sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i=0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    if (errno != ERANGE) {
        return lval_num(x);
    } else {
        return lval_err("Invalid number");
    }
}

lval* lval_read_dub(mpc_ast_t* t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    if (errno != ERANGE) {
        return lval_dub(x);
    } else {
        return lval_err("Invalid double");
    }
}

lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return lval_read_num(t);
    }
    if (strstr(t->tag, "double")) {
        return lval_read_dub(t);
    }
    if (strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
    }

    lval* x = NULL;
    /* if root (>) or sexpr then create empty list */
    if STR_EQ(t->tag, ">") {
        x = lval_sexpr();
    }

    if (strstr(t->tag, "sexpr")) {
        x = lval_sexpr();
    }

    if (strstr(t->tag, "qexpr")) {
        x = lval_qexpr();
    }

    for (int i=0; i < t->children_num; i++) {
        if STR_EQ(t->children[i]->contents, "(") { continue; }
        if STR_EQ(t->children[i]->contents, ")") { continue; }
        if STR_EQ(t->children[i]->contents, "}") { continue; }
        if STR_EQ(t->children[i]->contents, "{") { continue; }
        if STR_EQ(t->children[i]->tag, "regex") { continue; }

        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i=0; i < v->count; i++) {
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
        case LVAL_DUB:
            printf("%g", v->dub);
            break;
        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
        case LVAL_FUN:
            printf("<function>");
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
    }
}

void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

    v->count--;

    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

struct lenv {
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i=0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {
    /* linear scan, not cool bro. */
    for (int i=0; i < e->count; i++) {
        if STR_EQ(e->syms[i], k->sym) {
            return lval_copy(e->vals[i]);
        }
    }
    return lval_err("Unbound symbol '%s'!", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v) {
    for (int i=0; i < e->count; i++) {
        if STR_EQ(e->syms[i], k->sym) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* create new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

lval* builtin_op_num(lval* x, char* op, lval* y) {
    if ((STR_EQ("/", op) || STR_EQ("%", op)) && (y->num == 0)) {
        lval_del(x);
        lval_del(y);
        return lval_err("Divde By Zero!");
    }

    if (STR_EQ(op, "+")) {
        x->num += y->num;
    } else if (STR_EQ(op, "-")) {
        x->num -= y->num;
    } else if (STR_EQ(op, "*")) {
        x->num *= y->num;
    } else if (STR_EQ(op, "/")) {
        x->num /= y->num;
    } else if (STR_EQ(op, "%")) {
        x->num %= y->num;
    } else if (STR_EQ(op, "^")) {
        x->num = pow(x->num, y->num);
    } else if (STR_EQ(op, "min")) {
        x->num = MIN(x->num, y->num);
    } else if (STR_EQ(op, "max")) {
        x->num = MAX(x->num, y->num);
    } else {
        lval_del(x);
        lval_del(y);
        return lval_err("Invalid operator!");
    }
    lval_del(y);

    return x;
}

// still lots of duplication :(
lval* builtin_op_dub(lval* x, char* op, lval* y) {
    if ((STR_EQ("/", op) || STR_EQ("%", op)) && (y->dub == 0)) {
        lval_del(x);
        lval_del(y);
        return lval_err("Divide By Zero!");
    }

    if (STR_EQ(op, "+")) {
        x->dub += y->dub;
    } else if (STR_EQ(op, "-")) {
        x->dub -= y->dub;
    } else if (STR_EQ(op, "*")) {
        x->dub *= y->dub;
    } else if (STR_EQ(op, "/")) {
        x->dub /= y->dub;
    } else if (STR_EQ(op, "%")) {
        x->dub = fmod(x->dub, y->dub);
    } else if (STR_EQ(op, "^")) {
        x->dub = pow(x->dub, y->dub);
    } else if (STR_EQ(op, "min")) {
        x->dub = MIN(x->dub, y->dub);
    } else if (STR_EQ(op, "max")) {
        x->dub = MAX(x->dub, y->dub);
    } else {
        lval_del(x);
        lval_del(y);
        return lval_err("Invalid operator!");
    }
    lval_del(y);

    return x;
}

lval* coerce_num_to_dub(lval* n) {
    lval* d = lval_dub(n->num);
    lval_del(n);
    return d;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
    /* validate numbers */
    for (int i=0; i < a->count; i++) {
        if ((a->cell[i]->type != LVAL_NUM) &&
            (a->cell[i]->type != LVAL_DUB)) {
            lval_del(a);
            return lval_err("Cannot operator on non-number!");
        }
    }

    lval* x = lval_pop(a, 0);

    /* unary */
    if (STR_EQ(op, "-") && a->count == 0) {
        if (x->type == LVAL_NUM) {
            x->num = -x->num;
        } else {
            x->dub = -x->dub;
        }
    }

    while (a->count > 0) {
        lval* y = lval_pop(a, 0);

        // type coercion. doubles win.
        if (x->type != y->type) {
            if (x->type == LVAL_NUM) {
                x = coerce_num_to_dub(x);
            } else if (x->type == LVAL_DUB) {
                y = coerce_num_to_dub(y);
            }
        }

        if (x->type == LVAL_NUM) {
            x = builtin_op_num(x, op, y);
        } else if (x->type == LVAL_DUB) {
            x = builtin_op_dub(x, op, y);
        }
        if (x->type == LVAL_ERR) {
            break;
        }
    }

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

lval* builtin_mod(lenv* e, lval* a) {
    return builtin_op(e, a, "%");
}

lval* builtin_pow(lenv* e, lval* a) {
    return builtin_op(e, a, "^");
}

lval* builtin_min(lenv* e, lval* a) {
    return builtin_op(e, a, "min");
}

lval* builtin_max(lenv* e, lval* a) {
    return builtin_op(e, a, "max");
}

lval* builtin_head(lenv* e, lval* a) {
    LCHECK_COUNT("head", a, 1);
    LCHECK_TYPE("head", a->cell[0], LVAL_QEXPR);
    LCHECK_EMPTY("head", a->cell[0]);

    lval* v = lval_take(a, 0);
    while (v->count > 1) {
        lval_del(lval_pop(v, 1));
    }

    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    LCHECK_COUNT("tail", a, 1);
    LCHECK_TYPE("tail", a->cell[0], LVAL_QEXPR);
    LCHECK_EMPTY("tail", a->cell[0]);

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

// forward dec
lval* lval_eval(lenv* e, lval* v);

lval* builtin_eval(lenv* e, lval* a) {
    LCHECK_COUNT("eval", a, 1);
    LCHECK_TYPE("eval", a->cell[0], LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

// forward dec
lval* lval_join(lval* x, lval* y);

lval* builtin_join(lenv* e, lval* a) {
    for (int i=0; i < a->count; i++) {
        LCHECK_TYPE("join", a->cell[i], LVAL_QEXPR);
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
    LCHECK_COUNT("cons", a, 2);
    LCHECK_TYPE("cons", a->cell[1], LVAL_QEXPR);

    lval* v = lval_qexpr();
    lval* x = lval_pop(a, 0);
    lval* y = lval_pop(a, 0);

    v = lval_add(v,x);
    while (y->count) {
        v = lval_add(v, lval_pop(y, 0));
    }

    lval_del(a);

    return v;
}

lval* builtin_len(lenv* e, lval* a) {
    LCHECK_COUNT("len", a, 1);
    LCHECK_TYPE("len", a->cell[0], LVAL_QEXPR);

    lval* v = lval_num(0);
    lval* x = lval_take(a, 0);
    v->num = x->count;

    while (x->count) {
        lval_del(lval_pop(x, 0));
    }

    return v;
}

/* this name is funky. when I think of init I think of initialize. */
lval* builtin_init(lenv* e, lval* a) {
    LCHECK_COUNT("init", a, 1);
    LCHECK_TYPE("init", a->cell[0], LVAL_QEXPR);

    lval* x = lval_take(a, 0);

    /* if we get an empty list, return an empty list */
    if (x->count > 0) {
        lval_del(lval_pop(x, x->count-1));
    }

    return x;
}

lval* builtin_def(lenv* e, lval* a) {
    LCHECK_TYPE("def", a->cell[0], LVAL_QEXPR);

    lval* syms = a->cell[0];

    for (int i=0; i < syms->count; i++) {
        LCHECK(a, (syms->cell[i]-> type == LVAL_SYM), "Function 'def' cannot define non-symbol!");
    }

    LCHECK(a, (syms->count == a->count-1), "Function 'def' cannot define incorrect number of values to symbols!");

    for (int i=0; i < syms->count; i++) {
        lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpr();
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    /* Variable functions */
    lenv_add_builtin(e, "def", builtin_def);

    /* List functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "len", builtin_len);

    /* Math functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "^", builtin_pow);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);
}

lval* lval_eval_sexpr(lenv* e, lval* v) {

    for (int i=0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for (int i=0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    if (v->count == 0) {
        return v;
    }
    if (v->count == 1) {
        return lval_take(v, 0);
    }

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_del(v);
        lval_del(f);
        return lval_err("first element is not a function");
    }

    lval* result = f->fun(e, v);
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
    return v;
}

int main(int argc, char** argv) {
    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+d to Exit\n");

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Double   = mpc_new("double");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Qexpr    = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                  \
            double   : /-?[0-9]+\\.[0-9]+/ ;                                   \
            number   : /-?[0-9]+/ ;                                            \
            symbol   : /[a-zA-Z0-9_+i\\-*\\/%^\\\\=<>!&]+/;                    \
            sexpr    : '(' <expr>* ')';                                        \
            qexpr    : '{' <expr>* '}';                                        \
            expr     : <double> | <number> | <symbol> | <sexpr> | <qexpr> ;    \
            lispy    : /^/ <expr>* /$/ ;                                       \
            ",
            Number, Double, Symbol, Sexpr, Qexpr, Expr, Lispy);

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    while(1) {
        char* input = readline("lispy> ");
        add_history(input);
        if (!input) { break; }

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_read(r.output);
            x = lval_eval(e, x);
            lval_println(x);
            lval_del(x);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    lenv_del(e);

    mpc_cleanup(7, Number, Double, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
