#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 32;
  uint16_t CC = 100;
  for (uint16_t L = LL - 1; L > 0; L--) {
    for (uint16_t C = CC - 1; C > 0; C--) {
      float X = 0.0f, Y = 0.0f;
      float CX = C / (CC / 3.0f) - 2;
      float CY = L / (LL / 2.0f) - 1;
      uint16_t I = 10;
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
      printf("%c", I + 38);
    }
    printf("\n");
  }
  return 0;
}