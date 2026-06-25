#include <stdio.h>
#include <stdint.h>
int main() {
    uint16_t i = 0;
    while (i < 3) {
        i++;
        if (i == 2) continue;
        printf("%d\n", i);
    }
    return 0;
}
