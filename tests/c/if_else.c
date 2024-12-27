#include <stdio.h>
#include <stdint.h>

/*
test: Simple if-else statement
expect: STR__0^"Success">STR__1^"Failure">C^>C ,65C ?=(STR__0 PS:STR__1 PS)
input: A
output: Success
input: B
output: Failure
*/

int main() {
  uint8_t c;
  c = getchar();
  if (c == 'A') {
    printf("Success");
  } else {
    printf("Failure");
  }
  return 0;
}