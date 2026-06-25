#include <stdio.h>
int main() {
    int i = 0;
    while (i < 5) {
        i++;
        if (i % 2 == 0) continue;
        printf("%d\n", i);
    }
    return 0;
}
