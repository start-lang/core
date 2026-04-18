#include <stdint.h>
#include <start_tokens.h>

#ifndef START_LANG_H
#define START_LANG_H

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_SIZE 64

#define BL_PREV -1
#define BL_NEXT 0
#define SUCCESS 0
#define LOOP_ST 1
#define JM_NONE 0
#define JM_EIFE 1
#define JM_ENIF 2
#define JM_EWHI 3
#define JM_WHI0 4
#define JM_WHI1 5
#define JM_NXWH 6
#define JM_ECMT 7
#define JM_ELIN 8

#define JM_ERR0 9
#define JM_PEXC 10
#define JM_REOB 11
#define JM_EXEN 12

#define MAX_IDLEN 16

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
  uint8_t _id[MAX_IDLEN];
  uint8_t _stack_h;

  uint16_t _ans:1;
  uint16_t _type:2;
  uint16_t _forward:1;
  uint16_t _lookahead:1;
  uint16_t _op_result:4;
  uint16_t _string:1;
  uint16_t _srcinput:1;
  uint16_t _freesrc:1;
  uint16_t _cond:1;
  uint16_t _ignend:1;

  uint8_t _idlen;
  uint8_t _prev_token;
  int8_t _matching;

  uint16_t _lensrc;
  uint16_t _fmatching;

  struct _State * sub;

} State;

extern int8_t step_callback(State * s);

#define REG s->reg

int8_t st_run(State * s);

int8_t st_state_init(State * s);

int8_t st_step(State * s);

uint8_t st_op(uint8_t token, State * s);

void st_state_free(State * s);

typedef struct {
  uint8_t* name;
  int8_t (*fp)(State * s);
} Function;

extern Function ext[];

#ifdef __cplusplus
}
#endif

#endif
