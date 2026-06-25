#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t CC = 50;
  uint16_t C = 25;
  float CX = C / (CC / 3.0f) - 2;
  printf("%f\n", CX);
  return 0;
}
