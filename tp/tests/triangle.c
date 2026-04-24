#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t h = 6;
    for (uint16_t i = 1; i <= h; i++) {
        for (uint16_t s = 0; s < h - i; s++) {
            printf(" ");
        }
        for (uint16_t s = 0; s < 2 * i - 1; s++) {
            printf("*");
        }
        printf("\n");
    }
    return 0;
}
