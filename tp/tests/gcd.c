#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t a = 48;
    uint16_t b = 18;
    while (b != 0) {
        uint16_t t = b;
        b = a % b;
        a = t;
    }
    printf("%d\n", a);
    return 0;
}
