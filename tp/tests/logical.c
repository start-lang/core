#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t a = 5;
    uint16_t b = 10;
    if (a > 0 && b > 0) {
        printf("both positive\n");
    }
    if (a == 0 || b > 0) {
        printf("at least one\n");
    }
    if (!(a > b)) {
        printf("a not greater\n");
    }
    return 0;
}
