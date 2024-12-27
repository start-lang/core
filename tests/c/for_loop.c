#include <stdio.h>
#include <stdint.h>

/*
test: Test for loop
expect: C^0!>5C ?>[C PN C 1+5C ?>]
input:
output: 01234
*/

int main() {
  for (uint8_t c = 0; c < 5; c++) {
    printf("%d", c);
  }
  return 0;
}
