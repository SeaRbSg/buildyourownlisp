#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <editline/readline.h>
#include "mpc.h"

long eval_op(long x, char* op, long y) {
	if (strcmp(op, "+") == 0) { return x + y; }
	if (strcmp(op, "-") == 0) { return x - y; }
	if (strcmp(op, "*") == 0) { return x * y; }
	if (strcmp(op, "/") == 0) { return x / y; }
	if (strcmp(op, "%") == 0) { return x % y; }
	if (strcmp(op, "^") == 0) { return pow(x, y); }
	if (strcmp(op, "min") == 0) { return x < y ? x : y; }
	if (strcmp(op, "max") == 0) { return x > y ? x : y; }
	return 0;
}

long eval(mpc_ast_t* t) {
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	char* op = t->children[1]->contents;

	long x = eval(t->children[2]);

	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

  /* if '-' has only one argument, negate it. */
  if(strcmp(op, "-") == 0 && i == 3) {
    x = x * -1;
  }

	return x;
}

int main(int argc, char** argv) {
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
		" \
		  number : /-?[0-9]+/ ; \
		  operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
		  expr : <number> | '(' <operator> <expr>+ ')' ; \
		  lispy : /^/ <operator> <expr>+ /$/ ; \
		",
		Number, Operator, Expr, Lispy);

	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	while(1) {
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;
		if(mpc_parse("<stdin>", input, Lispy, &r)) {
			long result = eval(r.output);
			printf("%li\n", result);
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
