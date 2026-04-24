#include <stdio.h>

int main() {
    float x = 50.0f;
    float guess = x / 2.0f;
    for (int i = 0; i < 20; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    printf("%f\n", guess);
    return 0;
}
