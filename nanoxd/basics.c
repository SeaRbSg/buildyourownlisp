/* Variables */
int count;
int count = 10;

/* Function Declarations */
int add_together(int x, int y) {
  int result = x + y;
  return result;
}

int added = add_together(10, 18);


/* Structure Declarations */
typedef struct {
  float x;
  float y;
} point;

point p;
p.x = 0.1;
p.y = 10.0;

float length = sqrt(p.x * p.x + p.y * p.y);

/* Conditional */
int x = 12;
if (x > 10 && x < 100) {
  puts("x is greater than ten and less than one hundred!");
} else {
  puts("x is either less than eleven or greater than ninety-nine!");
}

/* Loops */
int i = 10;
while (i > 0) {
  puts("Loop Iteration");
  i = i - 1;
}

for (int j = 0; j < 10; j++) {
  puts("Loop Iteration");
}
