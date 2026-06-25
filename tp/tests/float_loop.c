#include <stdio.h>

int main() {
    float sum = 0.0f;
    for (int i = 0; i < 3; i++) {
        sum = sum + 1.5f;
    }
    printf("%f\n", sum);
    return 0;
}
