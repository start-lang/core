#include <stdio.h>
#include <stdint.h>

/*
test: Test while loop
expect: C^0!>5C ?>[C PN C 1+5C ?>]
input:
output: 01234
*/

int main() {
  uint8_t c = 0;
  while (c < 5) {
    printf("%d", c);
    c++;
  }
  return 0;
}
