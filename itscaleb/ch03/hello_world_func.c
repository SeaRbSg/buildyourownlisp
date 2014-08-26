#include <stdio.h>

void hello_n_times(int n) {
	for(int i = 0; i < n; i++) {
		puts("hello, world");
	}
}

int main(int argc, char** argv) {
	hello_n_times(5);
    return 0;
}
