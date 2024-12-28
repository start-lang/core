#include <stdio.h>
#include <stdint.h>

/*
test: Fibonacci sequence
expect: N^>A^0!>B^1!>T^>I^2!>N ,N 48N -0N ?=(0PN :1N ?=(1PN :N ;I ?g[A ;T !B ;T +B ;A !T ;B !I 1+N ;I ?g]B PN ))
input: 0
output: 0
input: 1
output: 1
input: 5
output: 5
input: 8
output: 21
*/

int main() {
  uint8_t n;
  uint8_t a = 0;
  uint8_t b = 1;
  uint8_t t;

  n = getchar();
  n = n - '0';

  if (n == 0) {
    printf("%d", 0);
  } else if (n == 1) {
    printf("%d", 1);
  } else {
    for (uint8_t i = 2; i <= n; i++) {
      t = a;
      t += b;
      a = b;
      b = t;
    }
    printf("%d", b);
  }
  return 0;
}