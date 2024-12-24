#include <Arduino.h>
#include <start_lang.h>
#include <microcuts.h>

uint8_t stop = 0;
uint16_t output_len = 0;

int8_t f_print(State * s);
int8_t f_printstr(State * s);
int8_t f_printnum(State * s);
int8_t f_debug_num(State * s);
int8_t exception_handler(State * s);
int8_t undef(State * s);
int8_t f_error(State * s);
int8_t f_quit(State * s);

#ifndef STDF_IGN_PRINT
int8_t f_print(State * s){
  output_len += Serial.printf("%c", (char) s->_m[0]);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTSTR
int8_t f_printstr(State * s){
  // string:str(_m) ... '/0'
  output_len += Serial.printf("%s", (char*) s->_m);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTNUM
int8_t f_printnum(State * s){
  // num:8/16/32/f(r0) -> _m:str
  char * m = (char *) s->_m;
  if (s->_type == INT8) s->reg.i8[0] = sprintf(m, "%d", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = sprintf(m, "%d", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = sprintf(m, "%d", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = sprintf(m, "%f", s->reg.f32);
  return 0;
}
#endif

#ifndef STDF_IGN_DEBUG_NUM
int8_t f_debug_num(State * s){
  // num:8/16/32/f(r0)
  if (s->_type == INT8) output_len += s->reg.i8[0] = printf("%d\n", s->reg.i8[0]);
  else if (s->_type == INT16) output_len += s->reg.i16[0] = printf("%d\n", s->reg.i16[0]);
  else if (s->_type == INT32) output_len += s->reg.i32 = printf("%d\n", s->reg.i32);
  else if (s->_type == FLOAT) output_len += s->reg.f32 = printf("%f\n", s->reg.f32);
  return 0;
}
#endif

#ifndef STDF_IGN_EXCEPTION_HANDLER
int8_t exception_handler(State * s){
  fprintf(stderr, "Something went wrong\n");
  return 0;
}
#endif

#ifndef STDF_IGN_UNDEF
int8_t undef(State * s){
  fprintf(stderr, "Undefined function or variable: %s\n", (char *) s->_id);
  return 0;
}
#endif

#ifndef STDF_IGN_ERROR
int8_t f_error(State * s){
  fprintf(stderr, "Raising error\n");
  return JM_ERR0;
}
#endif

#ifndef STDF_IGN_QUIT
int8_t f_quit(State * s){
  fprintf(stderr, "Quit\n");
  stop = 1;
  return 0;
}
#endif

#ifndef STDF_IGN_INPUT
int8_t f_input(State * s){
  s->_m[0] = getc(stdin);
  return 0;
}
#endif

#define STAR_T_FUNCTIONS             \
  {(uint8_t*)"", exception_handler}, \
  {(uint8_t*)"PRINT", f_print},      \
  {(uint8_t*)"PC", f_print},         \
  {(uint8_t*)"PRINTSTR", f_printstr},\
  {(uint8_t*)"PS", f_printstr},      \
  {(uint8_t*)"PRINTNUM", f_printnum},\
  {(uint8_t*)"PN", f_printnum},      \
  {(uint8_t*)"QUIT", f_quit},        \
  {(uint8_t*)"ERROR", f_error},      \
  {(uint8_t*)"DT", f_printstr},      \
  {(uint8_t*)"WS", f_printstr},      \
  {(uint8_t*)"IN", f_input},         \
  {(uint8_t*)"INPUT", f_input},      \
  {NULL, undef}                      \

int8_t step_callback(State * s) {
  return 0;
}

Function ext[] = {
  STAR_T_FUNCTIONS,
};
State * s;

int8_t run(char * src) {
  s = (State*) malloc(sizeof(State));
  memset((void*)s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = st_run(s);
  return result;
}


void clean(void){
  st_state_free(s);
  s = NULL;
}

char src[] = "s LL^32!>L^!> CC^100!>C^!> f CX^> CY^> TX^> TY^> TZ^> X^> Y^> bI^ sL 1- [ sCC;C! 1- [ b0I! f0X!Y! i0sCC;efCX!3/ i0sC;efCX@/ 2- i0sLL;efCY!2/ i0sL;efCY@/ 1- bI 10! [ f X;TX!* Y;TY!* 0TZ! TX;TZ+ TY;TZ+ 4?< ( x ) Y 2*  X;Y*  CY;Y+ TX;X! TY;X- CX;X+ bI 1- ?? ] bI;TZ! 38+ . sC 1- ?? ] b10TZ! . sL 1- ?? ]";

void setup() {
  Serial.begin(9600);
  int i = run(src);
  Serial.printf("i: %d\n", i);
  clean();
}

void loop() {
  sleep(1);
}
