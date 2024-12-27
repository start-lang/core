#include <stdio.h>
#include <stdint.h>

/*
test: Simple output with integer
expect: C^3!>C PN
input:
output: 3
*/

int main() {
  uint8_t c = 3;
  printf("%d", c);
  return 0;
}