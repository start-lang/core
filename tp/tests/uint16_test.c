#include <stdio.h>

int main() {
  printf("before\n");
  int I = 1;
  if (I == 0) {
    printf("zero\n");
  } else {
    printf("not zero\n");
  }
  printf("after\n");
  return 0;
}
