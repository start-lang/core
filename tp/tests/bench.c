#include <stdio.h>

int main() {
  int n;
  scanf("%d", &n);

  // fib
  int a = 0, b = 1;
  for (int i = 0; i < n; i++) {
    int t = a + b;
    a = b;
    b = t;
  }
  printf("%d\n", a);

  // primes
  int count = 0;
  for (int i = 2; i <= n; i++) {
    int prime = 1;
    for (int j = 2; j * j <= i; j++) {
      if (i % j == 0) { prime = 0; break; }
    }
    count += prime;
  }
  printf("%d\n", count);

  // collatz
  int total = 0;
  for (int i = 1; i <= n; i++) {
    int x = i;
    while (x != 1) {
      x = (x % 2 == 0) ? x / 2 : 3 * x + 1;
      total++;
    }
  }
  printf("%d\n", total);

  return 0;
}