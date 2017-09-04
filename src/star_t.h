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
  struct {
    uint8_t comp:1;
    uint8_t type:2;
    uint8_t i:2;
    uint8_t o:2;
  } info;
  uint8_t* mem;
} State;

#ifdef EXPOSE_INTERNALS
  extern State * s;
  extern uint8_t* mem_begin;
#endif

void begin();
int8_t run(uint8_t* code);

#ifdef __cplusplus
}
#endif

#endif
