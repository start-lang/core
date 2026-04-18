#include <stdint.h>
#include <stdlib.h>

#ifndef START_LANG_H
#define START_LANG_H

#ifdef __cplusplus
extern "C" {
#endif

#define INT8    0
#define INT16   1
#define INT32   2
#define FLOAT   3

#define MEM_SIZE 64

#ifndef ST_MAX_DEPTH
#define ST_MAX_DEPTH 32
#endif

#ifndef MAX_VARS
#define MAX_VARS 32
#endif

#ifndef MAX_FUNCS
#define MAX_FUNCS 32
#endif

#define SUCCESS 0
#define LOOP_ST 1
#define BL_PREV -1
#define BL_NEXT 0

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
#define MAX_BREAKS_PER_LOOP 32

typedef enum {
  // 0x00 — end of program
  OP_END          = 0x00,

  // 0x01–0x0F — control flow
  OP_IF           = 0x01,  // (
  OP_ELSE         = 0x02,  // :
  OP_ENDIF        = 0x03,  // )
  OP_WHILE        = 0x04,  // [
  OP_ENDWHILE     = 0x05,  // ]
  OP_BREAK        = 0x06,  // x
  OP_CONTINUE     = 0x07,  // c
  OP_RETURN       = 0x08,  // r
  OP_RUN          = 0x09,  // #

  // 0x10–0x1F — memory
  OP_LEFT         = 0x10,  // <
  OP_RIGHT        = 0x11,  // >
  OP_STORE        = 0x12,  // !
  OP_LOAD         = 0x13,  // ;
  OP_SWITCH       = 0x14,  // @
  OP_GTZERO       = 0x15,  // z
  OP_MALLOC       = 0x16,  // m
  OP_NEW_VAR      = 0x17,  // ^
  OP_FUNC_START   = 0x18,  // {
  OP_FUNC_END     = 0x19,  // }
  OP_ROTATE       = 0x1A,  // k
  OP_CAST         = 0x1B,  // e

  // 0x20–0x2A — math (including bitwise)
  OP_MATH_FIRST   = 0x20,
  OP_ADD          = 0x20,  // +
  OP_SUB          = 0x21,  // -
  OP_MUL          = 0x22,  // *
  OP_DIV          = 0x23,  // /
  OP_MOD          = 0x24,  // %
  OP_AND          = 0x25,  // w&
  OP_OR           = 0x26,  // w|
  OP_XOR          = 0x27,  // w^
  OP_NOT_BIT      = 0x28,  // w~
  OP_SHL          = 0x29,  // w<
  OP_SHR          = 0x2A,  // w>
  OP_MATH_LAST    = 0x2A,

  // 0x30–0x3F — comparisons
  OP_CMP_FIRST    = 0x30,
  OP_CMP_EQ       = 0x30,  // ?=
  OP_CMP_NEQ      = 0x31,  // ?!
  OP_CMP_LT       = 0x32,  // ?<
  OP_CMP_GT       = 0x33,  // ?>
  OP_CMP_LE       = 0x34,  // ?l
  OP_CMP_GE       = 0x35,  // ?g
  OP_CMP_NZ       = 0x36,  // ??
  OP_CMP_ZERO     = 0x37,  // ?z
  OP_CMP_STACK    = 0x38,  // ?h
  OP_CMP_TRUE     = 0x39,  // t
  OP_CMP_INV      = 0x3A,  // ~
  OP_CMP_LAST     = 0x3A,

  // 0x40–0x4F — type selection
  OP_TYPE_FIRST   = 0x40,
  OP_TYPE_I8      = 0x40,  // b
  OP_TYPE_I16     = 0x41,  // s
  OP_TYPE_I32     = 0x42,  // i
  OP_TYPE_F32     = 0x43,  // f
  OP_TYPE_LAST    = 0x43,

  // 0x50–0x5F — stack
  OP_PUSH         = 0x50,  // p
  OP_POP          = 0x51,  // o
  OP_STACK_H      = 0x52,  // h

  // 0x60–0x6F — I/O and strings
  OP_DEBUG        = 0x60,  // `
  OP_PRINT        = 0x61,  // .
  OP_INPUT        = 0x62,  // ,
  OP_STRING       = 0x63,  // "

  // 0x70–0x7F — constants
  OP_CONST_I8     = 0x70,
  OP_CONST_I16    = 0x71,
  OP_CONST_I32    = 0x72,
  OP_CONST_F32    = 0x73,

  // 0x80–0xBF — compact constants
  OP_CONST_MIN    = 0x80,
  OP_CONST_MAX    = 0xBF,

  // 0xC0–0xDF — variable access
  OP_VAR_FIRST    = 0xC0,
  OP_VAR_LAST     = 0xDF,

  // 0xE0–0xFF — function call
  OP_FUNC_FIRST   = 0xE0,
  OP_FUNC_LAST    = 0xFF,

} OpCode;

typedef struct {
  uint8_t* name;
  uint16_t pos;
} Variable;

typedef struct {
  uint8_t* name;
  uint8_t* src;
} RTFunction;

typedef union {
  uint8_t i8 [4];
  uint16_t i16 [2];
  uint32_t i32;
  float f32;
} Register;

typedef struct _State {
  Register reg;

  uint8_t* src;
  uint8_t* _src0;
  uint8_t* _m;
  uint8_t* _m0;
  uint16_t _mlen;
  uint8_t _stack_h;

  uint16_t _ans:1;
  uint16_t _type:2;
  uint16_t _forward:1;
  uint16_t _lookahead_unused:1; // kept for padding/source compatibility if needed
  uint16_t _op_result:4;
  uint16_t _string_unused:1;    // kept for padding
  uint16_t _srcinput_unused:1;  // kept for padding
  uint16_t _freesrc:1;
  uint16_t _cond:1;
  uint16_t _ignend:1;

  uint8_t _id[MAX_IDLEN];
  uint8_t _prev_token;
  uint8_t _matching;
  uint16_t _fmatching_unused;

  uint16_t _lensrc_unused;
  uint16_t _fmatching_old_unused;

  uint8_t _pending_char;
  uint32_t _pending_digit;
  uint8_t _has_pending_digit;
  struct _State * sub;

  uint16_t var_pos[MAX_VARS];
  uint8_t *func_src[MAX_FUNCS];
  uint16_t *func_jumps[MAX_FUNCS];
  uint8_t func_names[MAX_FUNCS][MAX_IDLEN];
  int8_t (*ext_fp[MAX_FUNCS])(struct _State * s);
  uint8_t varc;
  uint8_t funcc;
  uint16_t *_jumps;

} State;

extern int8_t step_callback(State * s);

#define REG s->reg

int8_t st_run(State * s);
int8_t st_state_init(State * s);
int8_t st_step(State * s);
uint8_t st_op(uint8_t op, State * s);
void st_state_free(State * s);
int8_t st_prepare(uint8_t *buf, uint16_t *jumps, uint16_t buf_len, State *s);

typedef struct {
  uint8_t* name;
  int8_t (*fp)(State * s);
} Function;

extern Function ext[];

#ifdef __cplusplus
}
#endif

#endif
