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

#define LCHECK_ALL_TYPES(func, arg, t) \
    for (int i=0; i < arg->count; i++) { \
        LCHECK_TYPE(func, arg->cell[i], t); \
    }

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
        case LVAL_DUB:
            return "Double";
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

    /* basic */
    long num;
    double dub;
    char* err;
    char* sym;

    /* function */
    lbuiltin builtin;
    char* fname;
    lenv* env;
    lval* formals;
    lval* body;

    /* expression */
    int count;
    lval** cell;
};

lval* lval_new(lval_type_t type) {
    lval* v = malloc(sizeof(lval));
    v->type = type;
    v->num = 0;
    v->dub = 0.0;
    v->err = NULL;
    v->sym = NULL;
    v->builtin = NULL;
    v->fname = NULL;
    v->env = NULL;
    v->formals = NULL;
    v->body = NULL;

    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_num(long x) {
    lval* v = lval_new(LVAL_NUM);
    v->num = x;
    return v;
}

lval* lval_dub(double x) {
    lval* v = lval_new(LVAL_DUB);
    v->dub = x;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval* v = lval_new(LVAL_ERR);

    va_list va;
    va_start(va, fmt);

    v->err = malloc(ERR_BUF_SIZE);
    vsnprintf(v->err, ERR_BUF_SIZE-1, fmt, va);

    v->err = realloc(v->err, strlen(v->err)+1);

    va_end(va);

    return v;
}

lval* lval_fun(char* s, lbuiltin builtin) {
    lval* v = lval_new(LVAL_FUN);
    v->builtin = builtin;
    v->fname = strdup(s);
    return v;
}

lenv* lenv_new(void);
void lenv_del(lenv* e);
lenv* lenv_copy(lenv* e);

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = lval_new(LVAL_FUN);
    v->env = lenv_new();
    v->body = body;
    v->formals = formals;
    return v;
}

lval* lval_sym(char* s) {
    lval* v = lval_new(LVAL_SYM);
    v->sym = strdup(s);
    return v;
}

lval* lval_sexpr(void) {
    return lval_new(LVAL_SEXPR);
}

lval* lval_qexpr(void) {
    return lval_new(LVAL_QEXPR);
}

void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_NUM:
        case LVAL_DUB:
            break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
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
    lval* x = lval_new(v->type);

    switch (v->type) {
        case LVAL_FUN:
            if (v->builtin) {
                x-> builtin = v->builtin;
                x->fname = strdup(v->fname);
            } else {
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;
        case LVAL_DUB:
            x->dub = v->dub;
            break;
        case LVAL_ERR:
            x->err = strdup(v->err);
            break;
        case LVAL_SYM:
            x->sym = strdup(v->sym);
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
            if (v->builtin) {
                printf("<%s>", v->fname);
            } else {
                printf("(\\");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
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
    lenv* parent;
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

/* so I guess this makes a copy of the current environment to create a
 * lexical scope within functions. Seems like this could happen later
 * (on writing new values) for a big performance gain for many functions.
 */
lenv* lenv_copy(lenv* e) {
    lenv* n = lenv_new();
    n->parent = e->parent;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for (int i=0; i < e->count; i++) {
        n->syms[i] = strdup(e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
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

void lenv_print_val(char* sym, lval* v) {
    printf("%s:\t", sym);
    lval_print(v);
    printf("\n");
}

void lenv_print(lenv* e) {
    for (int i=0; i < e->count; i++) {
        lenv_print_val(e->syms[i], e->vals[i]);
    }
}

lval* lenv_get(lenv* e, lval* k) {
    /* linear scan, not cool bro. */
    for (int i=0; i < e->count; i++) {
        if STR_EQ(e->syms[i], k->sym) {
            return lval_copy(e->vals[i]);
        }
    }
    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound symbol '%s'!", k->sym);
    }
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
    e->syms[e->count-1] = strdup(k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v) {
    while (e->parent) {
        e = e->parent;
    }
    lenv_put(e, k, v);
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
    LCHECK_ALL_TYPES("join", a, LVAL_QEXPR);

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

lval* builtin_env(lenv* e, lval* a) {
    LCHECK_COUNT("env", a, 1);
    LCHECK_TYPE("env", a->cell[0], LVAL_QEXPR);

    lval* syms = a->cell[0];
    LCHECK_ALL_TYPES("env", syms, LVAL_SYM);

    if (syms->count == 0) {
        lenv_print(e);
    } else {
        for (int i=0; i < syms->count; i++) {
            lval* v = lenv_get(e, syms->cell[i]);
            lenv_print_val(syms->cell[i]->sym, v);
        }
    }

    lval_del(a);

    return lval_sexpr();
}

lval* builtin_exit(lenv* e, lval* a) {
    LCHECK_COUNT("exit", a, 1);

    /* evaluates arguments and exits with the resulting error code */
    lval* x = lval_take(a, 0);
    lval* result = lval_eval(e, x);
    LCHECK_TYPE("exit", result, LVAL_NUM);
    printf("Goodbye, cruel world.\n");
    exit(result->num);

    /*  no cleanup or return because fuck it, we're exiting. */
}

lval* builtin_lambda(lenv* e, lval* a) {
    LCHECK_COUNT("\\", a, 2);
    LCHECK_TYPE("\\", a->cell[0], LVAL_QEXPR);
    LCHECK_TYPE("\\", a->cell[1], LVAL_QEXPR);
    LCHECK_ALL_TYPES("\\", a->cell[0], LVAL_SYM);

    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LCHECK_TYPE(func, a->cell[0], LVAL_QEXPR);
    lval* syms = a->cell[0];
    LCHECK_ALL_TYPES(func, syms, LVAL_SYM);
    LCHECK_COUNT(func, syms, a->count-1);

    for (int i=0; i < syms->count; i++) {
        if STR_EQ(func, "def") {
            lenv_def(e, syms->cell[i], a->cell[i+1]);
        } else if STR_EQ(func, "=") { /* you mean 'let' */
            lenv_put(e, syms->cell[i], a->cell[i+1]);
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

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(name, func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    /* REPL functions */
    lenv_add_builtin(e, "exit", builtin_exit);

    /* Variable functions */
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "env", builtin_env);

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

lval* lval_call(lenv* e, lval* f, lval* a);

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
        return lval_eval(e, lval_take(v, 0));
    }

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err(
                "S-expression starts with incorrect type. Got %s, Expected %s.",
                ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(v);
        lval_del(f);
        return err;
    }

    lval* result = lval_call(e, f, v);
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

lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->builtin) {
        return f->builtin(e, a);
    }

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. \
                    Got %i, Expected %i", given, total);
        }

        lval* sym = lval_pop(f->formals, 0);

        /* special case for variable arguments */
        if STR_EQ(sym->sym, "&") {
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
            }

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

    /* if '&' remains in formal list it should be bound to empty list */
    if (f->formals->count > 0 &&
            STR_EQ(f->formals->cell[0]->sym, "&")) {

        if (f->formals->count != 2) {
            return lval_err("Function formal invalid. Symbol '&' not followed by single symbol.");
        }

        lval_del(lval_pop(f->formals, 0));

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }  else { /* return partially evaluated function */
        return lval_copy(f);
    }
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
