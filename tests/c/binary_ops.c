#include <stdio.h>
#include <stdint.h>

/*
test: Test binary operators
expect: C^1!>C PN C 1C +C PN C 1C -C PN C 6C *C PN C 2C /C PN
input:
output: 12163
*/

int main() {
  uint8_t c = 1;
  printf("%d", c);
  c = c + 1;
  printf("%d", c);
  c = c - 1;
  printf("%d", c);
  c = c * 6;
  printf("%d", c);
  c = c / 2;
  printf("%d", c);
  return 0;
}