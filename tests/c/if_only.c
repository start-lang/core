#include <stdio.h>
#include <stdint.h>

/*
test: Simple if statement
expect: STR__0^"Success">C^>C ,65C ?=(STR__0 PS)
input: A
output: Success
input: B
output:
*/

int main() {
  uint8_t c;
  c = getchar();
  if (c == 'A') {
    printf("Success");
  }
  return 0;
}