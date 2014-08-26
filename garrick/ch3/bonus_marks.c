#include <stdio.h>

/* For loop to print 'Hello, world!' 5 times */
void forloop_hello_world() {
  int i = 0;
  puts("for:");
  for(int i = 0; i < 5; i++) {
    puts("Hello, world!");
  }
}

/* While loop to print 'Hello, world!' 5 times */
void whileloop_hello_world() {
  int i = 5;
  puts("while: ");
  while(i > 0) {
    puts("Hello, world!");
    i -= 1;
  }
}

/* 'Hello, world!' repeater */
void say_hello(int n) {
  for(int i = 0; i < n; i++) {
    puts("Hello, world!");
  }
}

int main(int argc, char** argv) {
  //forloop_hello_world();
  //whileloop_hello_world();
  say_hello(10);
  return 0;
}

/* Other types? 
  union
  arrays
*/

/* Other conditional operators?
    ==    Equal
    !=    Not equal
    >=    Greater than or equal
    <=    Less than or equal
    !a    Logical negation
    a &&  b Logical AND
    a ||  b Logical OR
 */

 /* Other mathematical operators? 
      +a      Unary plus
      -a      Unary minus
      a * b   multiplication
      a / b   division
      a % b   modulo
      ++a     pre-increment
      a++     post-increment
      --b     pre-decrement
      b--     post-decrement
  */

  /* what is the += operator and how does it work? 
      One of the compound assignment operators.  
      The statement: 

          a += b 
      
      is functionally equivelent to:

          a = a + b

      This meaning a is added to b and the result is assigned to a.
   */
   /* The do loop is used with while to build instrucitons where a set of operations are executed at least once since the while condition is evaluated AFTER the do block.  For example:

   */
   void dowhile_loop(int n) {
      do{
        puts("Don't just do something, stand there!"); 
      } while(n > 0);
   }
