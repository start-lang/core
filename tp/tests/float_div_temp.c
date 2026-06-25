#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t CC = 50;
  uint16_t C = 25;
  float temp = CC / 3.0f;
  float CX = C / temp - 2;
  printf("%f\n", CX);
  return 0;
}
