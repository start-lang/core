#include <stdio.h>
#include <stdint.h>

/*
test: Test assignment operators
expect: C^0!>C PN C 1!C PN C 1-C PN C 1+C PN C 2*C PN C 2/C PN
input:
output: 010121
*/

int main() {
  uint8_t c = 0;
  printf("%d", c);
  c = 1;
  printf("%d", c);
  c -= 1;
  printf("%d", c);
  c += 1;
  printf("%d", c);
  c *= 2;
  printf("%d", c);
  c /= 2;
  printf("%d", c);
  return 0;
}
