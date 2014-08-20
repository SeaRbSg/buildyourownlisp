#include<stdio.h>


int main() {
  /* Use a for loop to print out Hello World! five times. */
  for (int i = 0; i < 5; i++) {
    puts("Hello World!");
  }

  // Use a while loop to print out Hello World! five times.
  int j = 5;
  while (j > 0) {
    puts("Hello World!");
    j = j - 1;
  }

  hello_world_nth_times(7);
}

int hello_world_nth_times(int n) {
  while (n > 0) {
    puts ("Hello World from nth_times");
    n = n - 1;
  }
}
