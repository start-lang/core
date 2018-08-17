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
  Register A;
  Register B;
  uint8_t Ans:1;
  uint8_t Type:2;
  uint8_t Function;
  uint8_t* StackStart;
  uint8_t* Mem;
} State;

#ifdef EXPOSE_INTERNALS
  extern State * s;
  extern uint8_t* mem_begin;
#endif

void begin();
int8_t runall(uint8_t* code);
int8_t blockrun(uint8_t* block, uint8_t len);
int8_t step(uint8_t code);

extern void api(uint8_t pre, uint8_t op, State * s);

#ifdef __cplusplus
}
#endif

#endif
