#include <stdio.h>
#include <stdint.h>

int main() {
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
  printf("%d\n", I);
  return 0;
}
