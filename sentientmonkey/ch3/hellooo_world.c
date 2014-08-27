#include <stdio.h>

void hello(int n) {
    for (int i=0; i < n; i++) {
        puts("Hello, World!");
    }
}

int main(int argc, char** argv) {
    for (int i=0; i < 5; i++) {
        puts("Hello, World!");
    }

    puts("****************");

    int i=0;
    while (i < 5) {
        puts("Hello, World!");
        i++;
    }

    puts("****************");

    hello(1);

    puts("****************");

    hello(6);

    return 0;
}
