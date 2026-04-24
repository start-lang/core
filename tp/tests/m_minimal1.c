#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 4;
  uint16_t CC = 5;
  for (uint16_t L = LL - 1; L > 0; L--) {
    for (uint16_t C = CC - 1; C > 0; C--) {
      printf("%d ", C);
    }
    printf("\n");
  }
  return 0;
}
