#include <stdio.h>
#include <stdint.h>

/*
test: Test variable assignment
expect: A^1!>B^2!>T^>A PN B PN A ;T !T PN B ;A !T ;B !A PN B PN A ;B +B PN B ;A +A PN
input:
output: 1212135
*/

int main() {
  uint8_t a = 1;
  uint8_t b = 2;
  uint8_t t;
  printf("%d", a);
  printf("%d", b);
  t = a;
  printf("%d", t);
  a = b;
  b = t;
  printf("%d", a);
  printf("%d", b);
  b += a;
  printf("%d", b);
  a += b;
  printf("%d", a);
  return 0;
}
