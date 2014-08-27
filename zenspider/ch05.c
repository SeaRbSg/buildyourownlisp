#include "mpc.h"
#include <editline/readline.h>

int main() {
  mpc_parser_t* Adjective = mpc_new("adjective");
  mpc_parser_t* Noun      = mpc_new("noun");
  mpc_parser_t* Phrase    = mpc_new("phrase");
  mpc_parser_t* Doge      = mpc_new("doge");
  mpc_result_t r;

  mpca_lang(MPCA_LANG_DEFAULT, "                                                \
            adjective : \"wow\" | \"many\" | \"so\" | \"such\";                 \
            noun      : \"lisp\" | \"language\" | \"c\" | \"book\" | \"build\"; \
            phrase    : <adjective> <noun>;                                     \
            doge      : <phrase>*;                                              ",
            Adjective, Noun, Phrase, Doge);

  while (1) {
    char * input = readline("doge is stupid> ");
    if (!input) break;
    add_history(input);

    if (mpc_parse("<stdin>", input, Doge, &r)) {
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Adjective, Noun, Phrase, Doge);
}
