#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t i = 0;
    do {
        printf("%d\n", i);
        i++;
    } while (i < 5);
    return 0;
}
