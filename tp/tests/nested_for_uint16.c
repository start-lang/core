#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 5;
  uint16_t CC = 3;
  for (uint16_t L = LL - 1; L > 0; L--) {
    for (uint16_t C = CC - 1; C > 0; C--) {
      printf("%d %d\n", L, C);
    }
  }
  return 0;
}
