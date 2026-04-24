#include <stdio.h>

int main() {
  float X = 0.0f;
  float Y = 0.0f;
  uint16_t I = 3;
  while (1) {
    float TX = X * X;
    float TY = Y * Y;
    if (TX + TY > 4.0f) {
      break;
    }
    Y = 2.0f * X * Y + 0.5f;
    X = TX - TY + 0.5f;
    if (--I == 0) {
      break;
    }
  }
  printf("%d\n", I);
  return 0;
}
