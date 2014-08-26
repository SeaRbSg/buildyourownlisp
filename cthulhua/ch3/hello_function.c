#include <stdio.h>

void hello (int n) {
    for (int i = 0; i < n; ++i) {
        puts("Turn down for what!");
    }
}
int main (int argc, char** argv) {
    hello(7);
    return 0;
}
