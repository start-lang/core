#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 4;
  uint16_t CC = 5;
  for (uint16_t L = LL - 1; L > 0; L--) {
    for (uint16_t C = CC - 1; C > 0; C--) {
      float X = 0.5f;
      float Y = 0.5f;
      float CX = 1.0f;
      float CY = 0.5f;
      uint16_t I = 5;
      while (1) {
        float TX = X * X;
        float TY = Y * Y;
        if ((TX + TY > 4.0f)) {
          break;
        }
        Y = 2.0f * X * Y + CY;
        X = TX - TY + CX;
        if (--I == 0) {
          break;
        }
      }
      printf("%d ", I);
    }
    printf("\n");
  }
  return 0;
}
