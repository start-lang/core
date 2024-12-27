#include <stdio.h>
#include <stdint.h>

/*
test: Test unary operators
expect: C^0!>C PN C 1+C PN C 1-C PN C 1+C PN C 1-C PN
input:
output: 01010
*/

int main() {
  uint8_t c = 0;
  printf("%d", c);
  c++;
  printf("%d", c);
  c--;
  printf("%d", c);
  ++c;
  printf("%d", c);
  --c;
  printf("%d", c);
  return 0;
}