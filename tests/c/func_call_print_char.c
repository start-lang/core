#include <stdio.h>
#include <stdint.h>

/*
test: Simple output
expect: C^72!>C .
input:
output: H
*/

int main() {
  uint8_t c = 'H';
  printf("%c", c);
  return 0;
}