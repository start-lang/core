#include <stdio.h>
#include <stdint.h>

int main() {
  uint16_t LL = 32;
  for (uint16_t L = LL - 1; L > 0; L--) {
    printf("%d\n", L);
  }
  return 0;
}
