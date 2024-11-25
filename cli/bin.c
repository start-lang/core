#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <star_t.h>
#include <time.h>
#include <stdf.h>
#include <debug_utils.h>

State * s;
char * src;
uint8_t debug = 0;
uint8_t force_debug = 0;
uint8_t print_step_count = 1;
uint32_t max_steps = 0;
uint16_t max_output = 0;
uint64_t steps = 0;

int8_t step_callback(State * s) {
  debug_state(s, debug, debug);
  if (max_steps && steps >= max_steps) stop = 1;
  if (max_output && output_len >= max_output) stop = 1;
  steps++;
  return stop;
}

char * load_file(const char * fname){
  char *source = NULL;
  FILE *fp = fopen(fname, "r");
  if (fp != NULL) {
    if (fseek(fp, 0L, SEEK_END) == 0) {
      long bufsize = ftell(fp);
      if (bufsize == -1) { return NULL; }
      source = malloc(sizeof(char) * (bufsize + 1));
      if (fseek(fp, 0L, SEEK_SET) != 0) { free(source); return NULL; }
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      source[newLen++] = '\0';
    }
    fclose(fp);
    return source;
  } else {
    printf("Failed loading file\n");
    exit(1);
  }
  return NULL;
}

int8_t run(char * src) {
  stop = 0;
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = blockrun(s, 1);
  return result;
}

Function ext[] = {
  STAR_T_FUNCTIONS,
};

void help(char* cmd){
  printf("Usage: %s [ -h ] [ -f filename ] [ code ]", cmd);
  exit(0);
}

void arg_parse(int argc, char* argv[]){
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      help(argv[0]);
      exit(0);
    } else if (strcmp("-f", argv[i]) == 0 || strcmp("--file", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      src = load_file(argv[++i]);
      if (!src) {
        fprintf(stderr, "Failed loading file\n");
        exit(1);
      }
    } else if (strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
      debug = 1;
    } else if (strcmp("-S", argv[i]) == 0 || strcmp("--max-steps", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      max_steps = atoi(argv[++i]) * 1000;
    } else if (strcmp("-O", argv[i]) == 0 || strcmp("--max-output", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      max_output = atoi(argv[++i]);
    } else if (argv[i][0] != '-') {
      src = realloc(src, 1);
      src[0] = 0;
      for (int j = i; j < argc; j++) {
        int size = strlen(src) + strlen(argv[j]) + 1;
        src = realloc(src, size);
        strcat(src, argv[j]);
        strcat(src, " ");
      }
      break;
    }
  }
}

void remove_whitespace() {
    if (!src) return;
    char *p = src;
    char *q = src;
    int in_space = 0;
    while (*p) {
      if (*p == '\n') {
        p++;
      } else if (*p == ' ') {
        if (!in_space) {
          *q++ = ' ';
          in_space = 1;
        }
        p++;
      } else {
        *q++ = *p++;
        in_space = 0;
      }
    }
    *q = '\0';
}

int main(int argc, char* argv[]){
  setvbuf(stdout, NULL, _IONBF, 0);
  arg_parse(argc, argv);
  if (!src) {
    help(argv[0]);
    exit(1);
  }
  remove_whitespace();
  int8_t r = run(src);
  free_memory(s);
  if (print_step_count) {
    printf("%ld op\n", steps);
  }
  return r;
}