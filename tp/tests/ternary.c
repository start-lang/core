#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t a = 5;
    uint16_t b = 10;
    uint16_t max = a > b ? a : b;
    printf("%d\n", max);
    for (uint16_t i = 0; i < 5; i++) {
        uint16_t v = i % 2 == 0 ? 100 : 200;
        printf("%d\n", v);
    }
    return 0;
}
