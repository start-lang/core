#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 4;
  uint16_t CC = 5;
  for (uint16_t L = LL - 1; L > 0; L--) {
    for (uint16_t C = CC - 1; C > 0; C--) {
      float CX = C / (CC / 3.0f) - 2;
      float CY = L / (LL / 2.0f) - 1;
      printf("%f %f\n", CX, CY);
    }
  }
  return 0;
}
