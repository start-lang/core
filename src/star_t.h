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
  uint8_t* block;
  uint8_t* block_begin;

  #if defined(MAX_ITERATION_COUNT) || defined(PRINT_ITERATION_COUNT)
    uint16_t _ic;
  #endif

  uint8_t* mem_begin;
  uint8_t prev; // eq
  uint8_t lookahead; // change to bool :1
  int8_t stack; //eq
  int8_t inc; // pode ser foward (1) ou backward (-1) -> bool
  int8_t r;
} State;

int8_t blockrun(State * s);
int8_t step(uint8_t code, State * s);

extern void api(uint8_t pre, uint8_t op, State * s);

#ifdef __cplusplus
}
#endif

#endif
