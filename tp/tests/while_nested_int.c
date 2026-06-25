#include <stdio.h>
int main() {
    int i = 3;
    while (i > 0) {
        int j = 2;
        while (j > 0) {
            printf("%d %d\n", i, j);
            j--;
        }
        i--;
    }
    return 0;
}
