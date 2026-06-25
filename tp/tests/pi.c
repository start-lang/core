#include <stdio.h>

int main() {
    int c;
    int n;
    int i;
    int digit;
    int whole;
    double pi;
    double term;
    double sum1;
    double sum2;
    double frac;
    
    c = getchar();
    n = c - '0';
    
    if (n < 1 || n > 9) {
        n = 9;
    }
    
    sum1 = 0.0;
    term = 1.0 / 5.0;
    for (i = 0; i < 20; i = i + 1) {
        if (i % 2 == 0) {
            sum1 = sum1 + term;
        } else {
            sum1 = sum1 - term;
        }
        term = term * 1.0 / 25.0;
        term = term * (2 * i + 1);
        term = term / (2 * i + 3);
    }

    sum2 = 0.0;
    term = 1.0 / 239.0;
    for (i = 0; i < 20; i = i + 1) {
        if (i % 2 == 0) {
            sum2 = sum2 + term;
        } else {
            sum2 = sum2 - term;
        }
        term = term * 1.0 / (239.0 * 239.0);
        term = term * (2 * i + 1);
        term = term / (2 * i + 3);
    }

    pi = 16.0 * sum1 - 4.0 * sum2;

    whole = pi;
    digit = whole;
    putchar('0' + digit);
    putchar('.');
    
    frac = pi - whole;
    for (i = 0; i < n; i = i + 1) {
        frac = frac * 10.0;
        digit = frac;
        putchar('0' + digit);
        frac = frac - digit;
    }
    putchar('\n');
    
    return 0;
}
