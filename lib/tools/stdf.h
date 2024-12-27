#ifndef STDF_H
#define STDF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <start_lang.h>

uint8_t stop = 0;
uint16_t output_len = 0;

int8_t f_print(State * s);
int8_t f_printstr(State * s);
int8_t f_printnum(State * s);
int8_t exception_handler(State * s);
int8_t undef(State * s);
int8_t f_error(State * s);
int8_t f_quit(State * s);

#ifndef STDF_IGN_PRINT
int8_t f_print(State * s){
  output_len += printf("%c", (char) s->_m[0]);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTSTR
int8_t f_printstr(State * s){
  // string:str(_m) ... '/0'
  output_len += printf("%s", (char*) s->_m);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTNUM
int8_t f_printnum(State * s){
  // num:8/16/32/f(r0)
  if (s->_type == INT8) output_len += s->reg.i8[0] = printf("%d", s->reg.i8[0]);
  else if (s->_type == INT16) output_len += s->reg.i16[0] = printf("%d", s->reg.i16[0]);
  else if (s->_type == INT32) output_len += s->reg.i32 = printf("%d", s->reg.i32);
  else if (s->_type == FLOAT) output_len += s->reg.f32 = printf("%f", s->reg.f32);
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

#ifdef __cplusplus
}
#endif

#endif
