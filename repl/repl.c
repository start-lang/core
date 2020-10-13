#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <star_t.h>
#include <time.h>

//expf: run get_buffer clean
 
uint8_t ** M;
State * s;
char out[256] = "";
double time_spent;

int8_t run(char * src) {
  setvbuf(stdout, NULL, _IONBF, 0);
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  memset(out, 0, 256);
  s->src = (uint8_t*) src;
  clock_t begin = clock();
  int8_t result = blockrun(s, 1);
  clock_t end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  M = &(s->_m0);
  return result;
}

int8_t f_print(State * s){
  char tmp[2] = {(char) s->_m[0], 0};
  strcat(out, tmp);
  return 0;
}

int8_t f_printstr(State * s){
  strcat(out, (char*) s->_m);
  return 0;
}

int8_t f_printnum(State * s){
  if (s->_type == INT8) s->reg.i8[0] = sprintf(out, "%u", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = sprintf(out, "%u", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = sprintf(out, "%u", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = sprintf(out, "%f", s->reg.f32);
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

Function ext[] = {
  {(uint8_t*)"", exception_handler},
  {(uint8_t*)"PC", f_print}, 
  {(uint8_t*)"PS", f_printstr},
  {(uint8_t*)"PN", f_printnum},
  {(uint8_t*)"PRINT", f_print}, 
  {(uint8_t*)"PRINTSTR", f_printstr}, 
  {NULL, undef},
};

char * get_buffer(){
  return out;
}

void clean(){
  free_memory(s);
}
