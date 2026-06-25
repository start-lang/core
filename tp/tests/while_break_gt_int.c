#include <stdio.h>
int main() {
    int i = 0;
    while (1) {
        i++;
        if (i > 3) break;
        printf("%d\n", i);
    }
    return 0;
}
