#include <stdio.h>
#include <stdint.h>

/*
test: Simple variable declaration
expect: B0^>B1^1!>sS0^>S1^2!>iI0^>I1^3!>fF0^>F1^4!>
input:
output:
*/

int main() {
  uint8_t b0;
  uint8_t b1 = 1;
  uint16_t s0;
  uint16_t s1 = 2;
  uint32_t i0;
  uint32_t i1 = 3;
  float f0;
  float f1 = 4.0f;
  return 0;
}