#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

int main(int argc, char** argv) {
    mpc_parser_t* Key    = mpc_new("key");
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* String = mpc_new("string");
    mpc_parser_t* Naught = mpc_new("null");
    mpc_parser_t* Obj    = mpc_new("obj");
    mpc_parser_t* Array  = mpc_new("array");
    mpc_parser_t* AExpr  = mpc_new("aexpr");
    mpc_parser_t* Expr   = mpc_new("expr");
    mpc_parser_t* Json   = mpc_new("json");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                      \
            key         : /[a-z]+/ ;                                               \
            null        : \"null\" ;                                               \
            number      : /-?[0-9]+/ ;                                             \
            string      : '\"'/[a-zA-Z ]+/'\"' ;                                   \
            expr        : <obj> | <key> | <array> | <number> | <string> | <null> ; \
            obj         : '{' <key>':' <expr> '}';                                 \
            aexpr       : <expr>','? ;                                              \
            array       : '[' <aexpr> ']' ;                                       \
            json        : /^/ <obj> /$/ ;                                          \
            ",
            Naught, Key, Number, String, Obj, Array, AExpr, Expr, Json);

    puts("Json Version 0.0.1");
    puts("Press Ctrl+d to Exit\n");

    while(1) {
        char* input = readline("json> ");
        if (!input) {  break; }
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Json, &r)) {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(9, Naught, String, Number, Key, Obj, Array, AExpr, Expr, Json);

    return 0;
}
