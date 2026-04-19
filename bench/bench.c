// Standalone benchmark for start-lang interpreter.
// Measures execution throughput (mop/s) on two programs:
//   - fibonacci(46): pure arithmetic + tape walk, ~600 ops
//   - mandelbrot:    2D complex-plane loop, ~1.3M ops (optional, needs file)
//
// Usage:
//   make bench                            # fibonacci only
//   make bench BENCH_FILE=tests/mandelbrot/m.st
//
// Build:
//   make build-bench

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <start_lang.h>

static uint64_t g_ops = 0;

int8_t step_callback(State *s) {
  g_ops++;
  return 0;
}

int8_t f_input(State *s) { s->_m[0] = 0; return 0; }
int8_t f_print(State *s) { return 0; }
int8_t exception_handler(State *s) { return 0; }
int8_t undef(State *s) { return 0; }

Function ext[] = {
  {(uint8_t*)"",      exception_handler},
  {(uint8_t*)"PRINT", f_print},
  {NULL, NULL}
};

static uint64_t run_n(const char *src, int reps) {
  State s;
  uint64_t total = 0;
  for (int i = 0; i < reps; i++) {
    memset(&s, 0, sizeof(s));
    s.src = (uint8_t*)src;
    g_ops = 0;
    st_run(&s);
    total += g_ops;
  }
  return total;
}

static void bench(const char *name, const char *src, int reps) {
  int warmup = reps / 10 + 1;
  run_n(src, warmup);

  clock_t t0 = clock();
  uint64_t total_ops = run_n(src, reps);
  clock_t t1 = clock();

  double ms = (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC;
  double mops = (ms > 0) ? (double)total_ops / ms / 1000.0 : 0;

  printf("%-22s  %7d reps  %8.1f ms  %8.2f mop/s\n",
         name, reps, ms, mops);
}

static char *load_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  rewind(f);
  char *buf = malloc((size_t)sz + 1);
  if (!buf) { fclose(f); return NULL; }
  size_t n = fread(buf, 1, (size_t)sz, f);
  buf[n] = 0;
  fclose(f);
  return buf;
}

// fibonacci(46) via tape walk — tests integer arithmetic + while loops
#define FIB46 "i46!>0!>1!?=[2<1-?!2>;<@>+]"

int main(int argc, char **argv) {
  printf("start-lang benchmark\n");
  printf("%-22s  %7s  %10s  %12s\n", "program", "reps", "time", "throughput");
  printf("------------------------------------------------------------------\n");

  bench("fibonacci(46)", FIB46, 100000);

  if (argc > 1) {
    char *src = load_file(argv[1]);
    if (src) {
      bench(argv[1], src, 20);
      free(src);
    } else {
      printf("could not open: %s\n", argv[1]);
    }
  }

  printf("------------------------------------------------------------------\n");
  return 0;
}
