#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"

#define STR_EQ(A,B)   (strcmp(A,B) == 0)
#define MIN(X,Y)    ((X < Y) ? X : Y)
#define MAX(X,Y)    ((X > Y) ? X : Y)
#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

/* creating enums without typedef feels wrong, so I added them. */
typedef enum { LVAL_ERR, LVAL_NUM, LVAL_DUB, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR } lval_type_t;

typedef struct lval {
    lval_type_t type;
    long num;
    double dub;
    char* err;
    char* sym;
    int count;
    struct lval** cell;
} lval;

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

lval* builtin_op(lval* a, char* op) {
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

lval* builtin_head(lval* a) {
    LASSERT(a, (a->count == 1), "Function 'head' passed too many arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'head' passed incorrect type!");
    LASSERT(a, (a->cell[0]->count != 0), "Function 'head' passed {}!");

    lval* v = lval_take(a, 0);
    while (v->count > 1) {
        lval_del(lval_pop(v, 1));
    }

    return v;
}

lval* builtin_tail(lval* a) {
    LASSERT(a, (a->count == 1), "Function 'tail' passed too many arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'tail' passed incorrect type!");
    LASSERT(a, (a->cell[0]->count != 0), "Function 'tail' passed {}!");

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

// forward dec
lval* lval_eval(lval* v);

lval* builtin_eval(lval* a) {
    LASSERT(a, (a->count == 1), "Function 'eval' passed too many arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect type!");

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}

// forward dec
lval* lval_join(lval* x, lval* y);

lval* builtin_join(lval* a) {
    for (int i=0; i < a->count; i++) {
        LASSERT(a, (a->cell[i]->type == LVAL_QEXPR), "Function 'join' passed incorrect type!");
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
    LASSERT(a, (a->count == 2), "Function 'cons' passed incorrect number of arguments!");
    LASSERT(a, (a->cell[1]->type == LVAL_QEXPR), "Function 'cons' passed incorrect type!");

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

lval* builtin_len(lval* a) {
    LASSERT(a, (a->count == 1), "Function 'len' passed incorrect number of arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'len' passed incorrect type!");

    lval* v = lval_num(0);
    lval* x = lval_take(a, 0);
    v->num = x->count;

    while (x->count) {
        lval_del(lval_pop(x, 0));
    }

    return v;
}

/* this name is funky. when I think of init I think of initialize. */
lval* builtin_init(lval* a) {
    LASSERT(a, (a->count == 1), "Function 'init' passed incorrect number of arguments!");
    LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'init' passed incorrect type!");

    lval* x = lval_take(a, 0);

    /* if we get an empty list, return an empty list */
    if (x->count > 0) {
        lval_del(lval_pop(x, x->count-1));
    }

    return x;
}

lval* builtin(lval* a, char* func) {
    if STR_EQ("list", func) {
        return builtin_list(a);
    }
    if STR_EQ("head", func) {
        return builtin_head(a);
    }
    if STR_EQ("tail", func) {
        return builtin_tail(a);
    }
    if STR_EQ("join", func) {
        return builtin_join(a);
    }
    if STR_EQ("cons", func) {
        return builtin_cons(a);
    }
    if STR_EQ("init", func) {
        return builtin_init(a);
    }
    if STR_EQ("len", func) {
        return builtin_len(a);
    }
    if STR_EQ("eval", func) {
        return builtin_eval(a);
    }
    // allow for min/max. kind of a hack but hoping to clean up in next chapter
    if (strstr("+-/%*^", func) || STR_EQ("min", func) || STR_EQ("max", func)) {
        return builtin_op(a, func);
    }

    lval_del(a);
    return lval_err("Unknown Function!");
}

lval* lval_eval_sexpr(lval* v) {
    // eval children
    for (int i=0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    // error checking
    for (int i=0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    // empty expression
    if (v->count == 0) {
        return v;
    }

    // single expression
    if (v->count == 1) {
        return lval_take(v, 0);
    }

    // ensure first value is symbol
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression Does not start with symbol!");
    }

    lval* result = builtin(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval* v) {
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(v);
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
            symbol   : \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" | \"len\" | \"init\" | '+' | '-' | '*' | '/' | '%' | '^' |  \"min\" | \"max\"; \
            sexpr    : '(' <expr>* ')';                                        \
            qexpr    : '{' <expr>* '}';                                        \
            expr     : <double> | <number> | <symbol> | <sexpr> | <qexpr> ;    \
            lispy    : /^/ <expr>* /$/ ;                                       \
            ",
            Number, Double, Symbol, Sexpr, Qexpr, Expr, Lispy);

    while(1) {
        char* input = readline("lispy> ");
        add_history(input);
        if (!input) { break; }

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval* x = lval_read(r.output);
            x = lval_eval(x);
            lval_println(x);
            lval_del(x);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(7, Number, Double, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}
