#include <stdint.h>
#include <tokens.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STAR_T_H
#define STAR_T_H

#define MEM_SIZE 64

#define JM_NONE 0
#define JM_EIFE 1
#define JM_ENIF 2
#define JM_EWHI 3
#define JM_WHI0 4
#define JM_WHI1 5
#define JM_ERR0 6
#define JM_PEXC 7

#ifdef SIGNED_INT
  typedef int8_t byte_t;
  typedef int16_t int_t;
  typedef int32_t long_t;
#else
  typedef uint8_t byte_t;
  typedef uint16_t int_t;
  typedef uint32_t long_t;
#endif

typedef union {
  byte_t i8 [4];
  int_t i16 [2];
  long_t i32;
  float f32;
} Register;

typedef struct {
  uint8_t* name;
  uint16_t pos;
} Variable;

typedef struct {

  Register a;
  Register b;

  uint8_t* src;
  uint8_t* _src0;
  uint8_t* _m;
  uint8_t* _m0;
  uint8_t* _id;
  uint8_t* v_stack0; // TODO: remove virtual stack

  uint16_t _ic;

  uint8_t _ans:1;
  uint8_t _type:2;
  uint8_t _forward:1;
  uint8_t _lookahead:1;
  uint8_t  _prev_step_result:3;
  uint8_t  _string:1;

  uint8_t _idlen;
  uint8_t _prev_token;
  int8_t  _matching;

  Variable _vars[10];
  uint8_t _varc;
  
} State;

int8_t blockrun(State * s);
uint8_t step(uint8_t token, State * s);

extern void api(uint8_t pre, uint8_t op, State * s);

#ifdef __cplusplus
}
#endif

#endif
