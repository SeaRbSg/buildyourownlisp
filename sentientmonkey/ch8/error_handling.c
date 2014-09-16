#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <editline/readline.h>
#include "mpc.h"

#define STR_EQ(A,B)   (strcmp(A,B) == 0)
#define MIN(X,Y)    ((X < Y) ? X : Y)
#define MAX(X,Y)    ((X > Y) ? X : Y)

/* creating enums without typedef feels wrong, so I added them. */
typedef enum { LVAL_NUM, LVAL_DUB, LVAL_ERR } lval_type_t;
typedef enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_COERCE } lval_err_t;

/* Using union for value.
 * This allows us to save on storage space for our lval structures, at the cost of additional complexity in addressing them.
 */
typedef struct {
    lval_type_t type;
    union val {
        long num;
        double dub;
        lval_err_t err;
    } val;
} lval;

lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.val.num = x;
    return v;
}

lval lval_dub(double x) {
    lval v;
    v.type = LVAL_DUB;
    v.val.dub = x;
    return v;
}

lval lval_err(lval_err_t x) {
    lval v;
    v.type = LVAL_ERR;
    v.val.err = x;
    return v;
}

void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%li", v.val.num);
            break;
        case LVAL_DUB:
            printf("%g", v.val.dub);
            break;
        case LVAL_ERR:
            if (v.val.err == LERR_DIV_ZERO) { printf("Error: Division By Zero!"); }
            if (v.val.err == LERR_BAD_OP)   { printf("Error: Invalid Operator!"); }
            if (v.val.err == LERR_BAD_NUM)  { printf("Error: Invalid Number!"); }
            if (v.val.err == LERR_COERCE)   { printf("Error: Can't coerce types!"); }
            break;
        default:
            printf("Error: Unknown Operator!");
    }
}

void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if (x.type != y.type) { return lval_err(LERR_COERCE); }

    if ((x.type == LVAL_NUM) && (y.type == LVAL_NUM)) {
        if ((STR_EQ("/", op) || STR_EQ("%", op)) && (y.val.num == 0)) {
            return lval_err(LERR_DIV_ZERO);
        }

        if STR_EQ("+", op)   { return lval_num(x.val.num + y.val.num); }
        if STR_EQ("-", op)   { return lval_num(x.val.num - y.val.num); }
        if STR_EQ("*", op)   { return lval_num(x.val.num * y.val.num); }
        if STR_EQ("/", op)   { return lval_num(x.val.num / y.val.num); }
        if STR_EQ("%", op)   { return lval_num(x.val.num % y.val.num); }
        if STR_EQ("^", op)   { return lval_num(pow(x.val.num, y.val.num)); }
        if STR_EQ("min", op) { return lval_num(MIN(x.val.num, y.val.num)); }
        if STR_EQ("max", op) { return lval_num(MAX(x.val.num, y.val.num)); }

    } else if ((x.type == LVAL_DUB) && (y.type == LVAL_DUB)) {
        if ((STR_EQ("/", op) || STR_EQ("%", op)) && (y.val.dub == 0)) {
            return lval_err(LERR_DIV_ZERO);
        }

        if STR_EQ("+", op)   { return lval_dub(x.val.dub + y.val.dub); }
        if STR_EQ("-", op)   { return lval_dub(x.val.dub - y.val.dub); }
        if STR_EQ("*", op)   { return lval_dub(x.val.dub * y.val.dub); }
        if STR_EQ("/", op)   { return lval_dub(x.val.dub / y.val.dub); }
        if STR_EQ("%", op)   { return lval_dub(fmod(x.val.dub, y.val.dub)); }
        if STR_EQ("^", op)   { return lval_dub(pow(x.val.dub, y.val.dub)); }
        if STR_EQ("min", op) { return lval_dub(MIN(x.val.dub, y.val.dub)); }
        if STR_EQ("max", op) { return lval_dub(MAX(x.val.dub, y.val.dub)); }
    }

    return lval_err(LERR_BAD_OP);
}

lval eval_unary(lval x, char* op) {

    if (x.type == LVAL_NUM) {
        if (STR_EQ("-", op)) { return lval_num(x.val.num * -1); }
    } else if (x.type == LVAL_DUB) {
        if (STR_EQ("-", op)) { return lval_dub(x.val.dub * -1); }
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    if (strstr(t->tag, "double")) {
        errno = 0;
        double x = strtod(t->contents, NULL);
        return errno != ERANGE ? lval_dub(x) : lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    int i=3;

    if (t->children_num == i+1) {
        x = eval_unary(x, op);
    } else {
        while (strstr(t->children[i]->tag, "expr")) {
            x = eval_op(x, op, eval(t->children[i]));
            i++;
        }
    }

    return x;
}

int main(int argc, char** argv) {
    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+d to Exit\n");

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Double   = mpc_new("double");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                  \
            double   : /-?[0-9]+\\.[0-9]+/ ;                                   \
            number   : /-?[0-9]+/ ;                                            \
            operator : '+' | '-' | '*' | '/' | '%' | '^' |  \"min\" | \"max\"; \
            expr     : <double> | <number> | '(' <operator> <expr>+ ')' ;      \
            lispy    : /^/ <operator> <expr>+ /$/ ;                            \
            ",
            Number, Double, Operator, Expr, Lispy);

    while(1) {
        char* input = readline("lispy> ");
        add_history(input);
        if (!input) { break; }

        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(5, Number, Double, Operator, Expr, Lispy);
    return 0;
}
