#include <stdio.h>

int main() {
    float X = 1.0f;
    float Y = 2.0f;
    float CX = 0.5f;
    float CY = 0.3f;
    Y = 2.0f * X * Y + CY;
    X = X * X - Y * Y + CX;
    printf("%f\n", X);
    return 0;
}
