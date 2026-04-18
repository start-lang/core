#include <stdio.h>

int main() {
    int a = 0;
    int b = 1;
    for (int i = 0; i < 10; i++) {
        printf("%d\n", a);
        int temp = a + b;
        a = b;
        b = temp;
    }
    return 0;
}
