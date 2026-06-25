#include <stdio.h>

int main() {
    float X = 2.0f;
    float Y = 3.0f;
    float Z = X * X;
    X = Z - Y;
    printf("%f\n", X);
    return 0;
}
