#include <stdio.h>
#include <stdint.h>
int main() {
    uint16_t i = 3;
    while (i > 0) {
        uint16_t j = 2;
        while (j > 0) {
            printf("%d %d\n", i, j);
            j--;
        }
        i--;
    }
    return 0;
}
