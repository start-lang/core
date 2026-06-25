#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t result = 1;
    for (uint16_t i = 0; i < 3; i++) {
        result = result * 2;
    }
    printf("%d\n", result);
    return 0;
}
