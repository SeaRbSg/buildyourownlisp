#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int while_loop();
int for_loop();
int n_loop(int n);

// enter a number for a surprise
int main(int argc, char** argv){
  if (argc != 2){
    puts("That's rather unsportsmanlike. Please include input.");
    return 0;   
  }
  int n = atoi(argv[1]);
  switch(n){
    case 1:
      while_loop();
      break;
    case 4:
      for_loop();
      break;
    case 2:
    case 3:
    case 7:
    case 11:
    case 13:
    case 17:
    case 19:
      n_loop(n);
      break;
    case 1000:
      for_loop();
      while_loop();
    case 6:
    case 9:
    case 12:
    case 15:
    case 18:
      puts("What strange behavior.");
      break;
    case 27:
      puts("Oh! That's a good number.");
      break;
    case 100:
      puts("Starting countdown...");
      for(int i =100; i>0; i--){
        if(i == 92){
          puts("UGH, this'll take forever.");
          continue;
        }
        printf("%d...\n", i);
      }
      break;
    default:
      puts("Nope.");
      break;
  }
  return 0;   
}

int while_loop(){
  int i = 5;
  while(i > 0){
    puts("Hello, world.");
    i--;
  }
  return 0;
}

int for_loop(){
  for (int i=0; i<5; i++){
    puts("Hello, world.");
  }
  return 0;
}

int n_loop(int n){
  do{
    puts("Hello, world.");
    n--;
  } while (n != 0);
  return 0;
}
