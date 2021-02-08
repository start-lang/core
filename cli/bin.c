#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <star_t.h>
#include <time.h>

Register RM;
uint8_t * M;
State * s;
uint8_t stop = 0;
uint8_t debug = 0;

int8_t step_callback(State * s) {
  if (debug) {
    for (uint8_t * i = s->_m0; i <= s->_m + 2; i++){
      if (i == s->_m) {
        printf("[%d:%d:%d> ", REG.i32, s->_ans, s->_type);
      }
      printf("[%d] ", i[0]);
    }
    printf("\t\t- %c \n", s->src[0]);
  }
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
  }
  return NULL;
}

int8_t run(char * src) {
  stop = 0;
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = blockrun(s, 1);
  RM.i8[0] = s->_m[0];
  RM.i8[1] = s->_m[1];
  RM.i8[2] = s->_m[2];
  RM.i8[3] = s->_m[3];
  M = s->_m0;
  return result;
}

int8_t f_print(State * s){
  printf("%c", (char) s->_m[0]);
  return 0;
}

int8_t f_printstr(State * s){
  // string:str(_m) ... '/0'
  printf("%s\n", (char*) s->_m);
  return 0;
}

int8_t f_printnum(State * s){
  // num:8/16/32/f(r0) -> _m:str
  char * m = (char *) s->_m;
  if (s->_type == INT8) s->reg.i8[0] = sprintf(m, "%d", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = sprintf(m, "%d", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = sprintf(m, "%d", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = sprintf(m, "%f", s->reg.f32);
  return 0;
}

int8_t f_debug_num(State * s){
  // num:8/16/32/f(r0)
  if (s->_type == INT8) s->reg.i8[0] = printf("%d\n", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = printf("%d\n", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = printf("%d\n", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = printf("%f\n", s->reg.f32);
  return 0;
}

int8_t exception_handler(State * s){
  fprintf(stderr, "Something went wrong\n");
  return 0;
}

int8_t undef(State * s){
  fprintf(stderr, "Undefined function or variable: %s\n", (char *) s->_id);
  return 0;
}

int8_t f_error(State * s){
  fprintf(stderr, "Raising error\n");
  return JM_ERR0;
}

int8_t f_quit(State * s){
  fprintf(stderr, "Quit\n");
  stop = 1;
  return 0;
}

Function ext[] = {
  {(uint8_t*)"", exception_handler},
  {(uint8_t*)"PC", f_print},
  {(uint8_t*)"PS", f_printstr},
  {(uint8_t*)"PN", f_printnum},
  {(uint8_t*)"PRINT", f_print},
  {(uint8_t*)"PRINTSTR", f_printstr},
  {(uint8_t*)"PRINTNUM", f_printnum},
  {(uint8_t*)"QUIT", f_quit},
  {(uint8_t*)"ERROR", f_error},
  {(uint8_t*)"DT", f_printstr},
  {(uint8_t*)"WS", f_printstr},
  {NULL, undef},
};

void help(char* cmd){
  printf("Usage: %s [ -h ] [ -f filename ] [ code ]", cmd);
  exit(0);
}

int main(int argc, char* argv[]){
  setvbuf(stdout, NULL, _IONBF, 0);

  if (argc == 1) {
    help(argv[0]);
    return 0;
  } else {
    if (strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0) {
      help(argv[0]);
      return 0;
    } else if (strcmp("-f", argv[1]) == 0 || strcmp("--file", argv[1]) == 0) {
      if (argc != 3) {
        help(argv[0]);
      }
      char * src = load_file(argv[2]);
      if (!src) {
        fprintf(stderr, "Failed loading file\n");
        exit(0);
      }
      int8_t r = run(src);
      free_memory(s);
      return r;
    }
    int pos = 1;
    if (strcmp("-d", argv[1]) == 0 || strcmp("--debug", argv[1]) == 0) {
      pos++;
      debug = 1;
    }

    int size = 0;
    char * src = malloc(1);
    src[0] = 0;
    for (int i = pos; i < argc; i++) {
      size += strlen(argv[i]) + 1;
      src = realloc(src, size);
      strcat(src, argv[i]);
      strcat(src, " ");
    }
    printf("%s\n", src);
    int8_t r = run(src);
    free_memory(s);
    return r;
  }
  return 0;
}