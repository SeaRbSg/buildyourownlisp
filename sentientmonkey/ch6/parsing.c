#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include "mpc.h"

int main(int argc, char** argv) {
    puts("RPN Version 0.0.1");
    puts("Press Ctrl+d to Exit\n");

    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                       \
            number   : /-?[0-9]+(\\.[0-9]+)?/ ;                                     \
            operator : '+' | '-' | '*' | '/' | '%' | \"add\" | \"mul\" | \"div\" ;  \
            expr     : <number> | '(' <operator> <expr>+ ')' ;                      \
            lispy    : /^/ <operator> <expr>+ /$/ ;                                 \
            ",
            Number, Operator, Expr, Lispy);

    while(1) {

        char* input = readline("rpn> ");
        add_history(input);

        // break if we don't have input
        if (!input) { break; }
        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}
