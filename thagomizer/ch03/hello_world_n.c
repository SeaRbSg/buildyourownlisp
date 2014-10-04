#include <stdio.h>

void hello_n(int n) {
  for (int i = 0; i < n; i++) {
    puts("Hello, World!");
  }
  return;
}

int main(int argc, char** argv) {
  hello_n(7);
  return 0;
}

