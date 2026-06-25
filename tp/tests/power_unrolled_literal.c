#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t result = 1;
    result = result * 2;
    result = result * 2;
    result = result * 2;
    printf("%d\n", result);
    return 0;
}
