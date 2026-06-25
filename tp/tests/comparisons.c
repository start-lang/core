#include <stdio.h>

int main() {
    int a = 5;
    int b = 5;
    if (a == b) {
        printf("eq\n");
    }
    if (a != 10) {
        printf("neq\n");
    }
    if (a > 3) {
        printf("gt\n");
    }
    if (a >= 5) {
        printf("ge\n");
    }
    if (a <= 5) {
        printf("le\n");
    }
    return 0;
}
