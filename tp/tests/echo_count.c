#include <stdio.h>
#include <stdint.h>

int main() {
    uint16_t count = 0;
    int c;
    while ((c = getchar()) != -1 && c != 10) {
        putchar(c);
        count++;
    }
    putchar('\n');
    printf("%d\n", count);
    return 0;
}
