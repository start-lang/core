#include <stdint.h>
#include <tokens.h>

#ifndef STAR_T_H
#define STAR_T_H

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_SIZE 64

#define JM_NONE 0
#define JM_EIFE 1
#define JM_ENIF 2
#define JM_EWHI 3
#define JM_WHI0 4
#define JM_WHI1 5
#define JM_ERR0 6
#define JM_PEXC 7

typedef union {
  uint8_t i8 [4];
  uint16_t i16 [2];
  uint32_t i32;
  float f32;
} Register;

typedef struct {
  uint8_t* name;
  uint16_t pos;
} Variable;

typedef struct {
  uint8_t* name;
  uint8_t* src;
} RTFunction;

typedef struct _State {
  Register reg;

  uint8_t* src;
  uint8_t* _src0;
  uint8_t* _m;
  uint8_t* _m0;
  uint16_t _mlen;
  uint8_t* _id;
  uint8_t* v_stack0; // TODO: remove virtual stack

  uint16_t _ic;

  uint8_t _ans:1;
  uint8_t _type:2;
  uint8_t _forward:1;
  uint8_t _lookahead:1;
  uint8_t  _prev_step_result:3;
  uint8_t  _string:1;
  uint8_t  _srcinput:1;

  uint8_t _idlen;
  uint8_t _prev_token;
  int8_t  _matching;

  Variable * _vars;
  uint8_t _varc;

  RTFunction * _funcs;
  uint8_t _funcc;

  uint16_t _lensrc;
  uint16_t _fmatching;
  
  struct _State * sub;
  
} State;


#define RM ((Register*)s->_m)[0]
#define REG s->reg

int8_t blockrun(State * s, uint8_t last);
uint8_t step(uint8_t token, State * s);
void free_memory(State * s);

typedef struct {
  uint8_t* name;
  int8_t (*fp)(State * s);
} Function;

extern Function ext[];

extern void api(uint8_t pre, uint8_t op, State * s);

#ifdef __cplusplus
}
#endif

#endif
