#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>


#ifdef _WIN32

#include <string.h>

//declare static buffer "input" for user input
static char input[2048];

//readline for windows: does not allow use of arrow keys
char* readline(char* prompt) {
      fputs("rhl> ", stdout);
      fgets(input, 2048, stdin);
      char* cpy = malloc(strlen(input)+1);
      strcpy(cpy, input);
      cpy[strlen(cpy)-1] = '\0';
      return cpy;
}

//faux add_history for windows
void add_history(char* unused) {}

#else

// to allow for use of arrow keys and prior input access in prompt
#include <editline/readline.h>

#endif

typedef struct {
		int type;
		long num;
		int err;
} lval;

// enumeration of possible lval types
enum { LVAL_NUM, LVAL_ERR };

//enumercation of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x){
	lval v; 
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

lval lval_err(int x){
	lval v; 
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

//print lval
void lval_print(lval v) {
	switch (v.type){
		case LVAL_NUM: printf("%li", v.num);
			break;
		case LVAL_ERR:
			if (v.err == LERR_DIV_ZERO){
					printf("Error: Divide by zero");
			}
			if (v.err == LERR_BAD_OP){
					printf("Error: Invalid operator");
			}
			if (v.err == LERR_BAD_NUM){
					printf("Error: Invalid number");
			}
			break;
	}
}

//print lval followed by newline
void lval_println(lval v){
	lval_print(v);
	putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
	if (x.type == LVAL_ERR){
			return x;
	}
	if (y.type == LVAL_ERR){
			return y;
	}

	if (strcmp(op, "+") == 0) {
			return lval_num(x.num + y.num); 
	}
	if (strcmp(op, "-") == 0) {
			return lval_num(x.num - y.num); 
	}
	if (strcmp(op, "*") == 0) {
			return lval_num(x.num * y.num); 
	}
	if (strcmp(op, "/") == 0) {
			return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); 
	}
	if (strcmp(op, "%") == 0) {
			return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num); 
	}
	if (strcmp(op, "^") == 0) {
			return lval_num(pow(x.num, y.num)); 
	}
	if (strcmp(op, "min") == 0) {
			return x.num < y.num ? lval_num(x.num) : lval_num(y.num);  
	}
	if (strcmp(op, "max") == 0) {
			return x.num > y.num ? lval_num(x.num) : lval_num(y.num);  
	}

	//else, I have no idea wtf you're saying
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){

	// return directly if tagged as number
	if (strstr(t->tag, "number")) {
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	char* op = t->children[1]->contents;
	lval x = eval(t->children[2]);

	//iterate over remaining children
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")){
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	if(i == 3){
		// did not increment; no third child; op is negation
		x = lval_num(x.num * -1);
	}
	
	return x;

}

int main(int argc, char** argv){
 
  // Polish notation parser

  // create parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  // Define parsers
  mpca_lang(MPCA_LANG_DEFAULT,
     "                                                  							  \
     number   : /-?[0-9]+/ ;                             								\
     operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;     \
     expr     : <number> | '(' <operator> <expr>+ ')' ;  								\
     lispy    : /^/ <operator> <expr>+ /$/ ;             								\
     ",
     Number, Operator, Expr, Lispy);


  // Begin prompt
  puts("Rustic Homemade Lisp-thing version 0.0.0.0.1");
  puts("Press Crtl+c to exit\n");

  // input loop
  while (1){
    
    // output prompt text and get input
    char* input = readline("rhl> ");
    
    add_history(input);
    
    // parse input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){
				// legit input: eval and print result. Delete tree after evaluating.
				lval result = eval(r.output);
        lval_println(result);
        mpc_ast_delete(r.output);
    }
    else {
        // failure: print error
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
    
    free(input);
  }

  // Undefine and Delete Parsers
  mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}



