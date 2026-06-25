#include <stdio.h>
#include <stdint.h>

int main() {
  uint32_t n = 2;
  uint32_t mask = 1;
  uint32_t sum = 0;

  while (1) {
    uint32_t d = 2;
    uint8_t is_prime = 1;

    while (d < n) {
      uint32_t q = 0;
      for (uint32_t i = 0; i < n; i++) q++;

      while (q >= d) {
        for (uint32_t i = 0; i < d; i++) q--;
      }

      if (q == 0) {
        is_prime = 0;
        break;
      }
      d++;
    }

    if (is_prime) {
      for (uint32_t i = 0; i < n; i++) sum++;
      if (sum > mask) {
        mask <<= 1;
        if (mask == 0) {
          printf("done\n");
          break;
        }
        printf("%d\n", n);
      }
    }
    n++;
  }

  return 0;
}
