#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <start_lang.h>
#include <time.h>
#include <stdf.h>
#include <debug_utils.h>

State * s;
char * src;
uint8_t debug = 0;
uint8_t interactive = 0;
uint8_t exec_info = 0;
extern uint32_t timeout;
extern uint32_t max_steps;
extern uint16_t max_output;
extern uint8_t follow_vars;
extern uint8_t follow_mem;
extern uint8_t first_callback;

int8_t step_callback(State * s) {
  return stop || debug_state(s, debug, interactive);
}

#ifdef ENABLE_FILES
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
#endif

int8_t run(char * src) {
  stop = 0;
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = st_run(s);
  return result;
}

Function ext[] = {
  STAR_T_FUNCTIONS,
};

void help(char* cmd) {
  printf(
    "Usage: %s [OPTIONS] [CODE]\n\n"
    "Executes the provided code or loads it from a file.\n\n"
    "Options:\n"
    "  -h, --help           Show this help message and exit\n"
    "  -f, --file FILE      Load source code from FILE\n"
    "  -d, --debug          Enable step-by-step debug mode\n"
    "  -i, --interactive    Enable interactive debug mode\n"
    "  -e, --exec-info      Print execution statistics\n"
    "  -S, --max-steps N    Set execution step limit to N * 1000\n"
    "  -O, --max-output N   Set text output limit to N characters\n"
    "  -t, --timeout N      Set execution timeout to N * 1000 ms (default: 3000)\n"
    "  -s, --src            Print processed source code before execution\n"
    "  -v, --version        Print version and exit\n"
    "  -,  --stdin          Read source code from standard input\n\n"
    "Examples:\n"
    "  %s '\"Hello, World!\" PS'                # Print string\n"
    "  %s -f quine.st                         # Run code from a file\n"
    "  %s -i '8!>0!>1!?=[2<1-?!2>;<@>+] ;PN'  # 8th Fibonacci number\n"
    "  %s '>,[>,]<[.<]'                       # Reverse input (ESC+Enter to end)\n"
    "  echo \"-[----->+<]>++++.\" | %s -e -     # Print 7\n\n"
    "Notes:\n"
    "  - Default limit is 2.5M steps or 1000 steps in debug mode\n"
    "  - Default timeout is 3s unless specified.\n",
    cmd, cmd, cmd, cmd, cmd, cmd
  );
  exit(0);
}

void reset_env() {
  debug = 0;
  interactive = 0;
  exec_info = 0;
  follow_vars = 0;
  follow_mem = 0;
  timeout = 0;
  max_steps = 0;
  max_output = 0;
  first_callback = 1;
  src = realloc(src, 1);
  src[0] = 0;
}

#ifndef VERSION
#define VERSION "0.0.0"
#endif

#ifndef COMMIT
#define COMMIT "nightly"
#endif

void arg_parse(int argc, char* argv[]){
  reset_env();
  uint8_t print_src = 0;
  if (argc == 1) {
    help(argv[0]);
    exit(1);
  }
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      help(argv[0]);
      exit(0);
#ifdef ENABLE_FILES
    } else if (strcmp("-f", argv[i]) == 0 || strcmp("--file", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      src = load_file(argv[++i]);
      if (!src) {
        fprintf(stderr, "Failed loading file\n");
        exit(1);
      }
#else
    } else if (strcmp("-f", argv[i]) == 0 || strcmp("--file", argv[i]) == 0) {
      fprintf(stderr, "Files not enabled\n");
      exit(1);
#endif
    } else if (strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
      debug = 1;
      follow_vars = 1;
    } else if (strcmp("-S", argv[i]) == 0 || strcmp("--max-steps", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      max_steps = atoi(argv[++i]) * 1000;
    } else if (strcmp("-O", argv[i]) == 0 || strcmp("--max-output", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      max_output = atoi(argv[++i]);
    } else if (strcmp("-i", argv[i]) == 0 || strcmp("--interactive", argv[i]) == 0) {
      interactive = 1;
      debug = 1;
      follow_vars = 1;
    } else if (strcmp("-e", argv[i]) == 0 || strcmp("--exec-info", argv[i]) == 0) {
      follow_mem = 1;
      exec_info = 1;
    } else if (strcmp("-t", argv[i]) == 0 || strcmp("--timeout", argv[i]) == 0) {
      if (i + 1 >= argc) { help(argv[0]); exit(1); }
      timeout = atoi(argv[++i]) * 1000;
    } else if (strcmp("-s", argv[i]) == 0 || strcmp("--src", argv[i]) == 0) {
      print_src = 1;
    } else if (strcmp("-v", argv[i]) == 0 || strcmp("--version", argv[i]) == 0) {
      printf("*T lang %s - %s\n", VERSION, COMMIT);
      printf("%s\n", __VERSION__);
      printf("%s\n", __DATE__);
      exit(0);
    } else if (strcmp("-", argv[i]) == 0 || strcmp("--stdin", argv[i]) == 0) {
      char c;
      while ((c = getc(stdin)) != EOF) {
        int size = strlen(src) + 2;
        src = realloc(src, size);
        src[size - 2] = c;
        src[size - 1] = 0;
      }
    } else {
      int size = strlen(src) + strlen(argv[i]) + 1;
      src = realloc(src, size);
      strcat(src, argv[i]);
      strcat(src, " ");
    }
  }

  if (print_src) {
    printf("%s\n", src);
  }

  if (!max_steps) {
    if (debug && !interactive) {
      max_steps = 1000;
    } else {
      max_steps = 2500000;
    }
  }
  if (!timeout) {
    timeout = 3000;
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
  st_state_free(s);
  if (exec_info) {
    print_exec_info();
  }
  return r;
}

#ifdef EXPORT
int main_init(int argc, char* argv[]){
  setvbuf(stdout, NULL, _IONBF, 0);
  arg_parse(argc, argv);
  if (!src) {
    help(argv[0]);
    exit(1);
  }
  remove_whitespace();
  stop = 0;
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  st_state_init(s);
  return 0;
}

int main_step(void){
  State * sub = s;
  while (sub->sub) sub = sub->sub;
  if (step_callback(sub) != 0){
    return LOOP_ST; // TODO change return val?
  }
  return st_step(s);
}

void main_free(void){
  st_state_free(s);
  if (exec_info) {
    print_exec_info();
  }
}
#endif
