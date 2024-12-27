#include <stdio.h>
#include <stdint.h>

/*
test: Simple input and output
expect: C^>C ,C .
input: I
output: I
*/

int main() {
  uint8_t c;
  c = getchar();
  printf("%c", c);
  return 0;
}