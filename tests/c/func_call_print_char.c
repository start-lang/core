#include <stdio.h>
#include <stdint.h>

/*
test: Simple output with char
expect: C^72!>C .
input:
output: H
*/

int main() {
  uint8_t c = 'H';
  printf("%c", c);
  return 0;
}