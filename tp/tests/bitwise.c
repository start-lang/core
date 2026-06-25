#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t a = 12;
    uint16_t b = 10;
    printf("%d\n", a & b);
    printf("%d\n", a | b);
    printf("%d\n", a ^ b);
    printf("%d\n", a << 2);
    printf("%d\n", a >> 1);
    return 0;
}
