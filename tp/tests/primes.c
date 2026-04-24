#include <stdio.h>
#include <stdint.h>

int main() {
    for (uint16_t n = 2; n < 30; n++) {
        uint16_t is_prime = 1;
        for (uint16_t d = 2; d * d <= n; d++) {
            if (n % d == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) {
            printf("%d ", n);
        }
    }
    printf("\n");
    return 0;
}
