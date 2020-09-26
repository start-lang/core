#include <stdlib.h>
#include <string.h>
#include <star_t.h>

#ifdef DEBUG_INTERPRETER
#   include <stdio.h>
#   include <microcuts.h>
// #   define pe() __parsing_error(__FILE__, __LINE__, (const char*) code_begin); return -1;
//     void __parsing_error(const char* file, int line, const char* code_begin, const char* pos){
//       int at = (int)(pos - code_begin);
//       char token = *(pos);
//       printf("Parsing error: Unexpected token '%c' at position %d on file '%s', line %d.\n", token, at, file, line);
//       printf("\t%s\n\t", code_begin);
//       while(at){
//         at--;
//         printf(" ");
//       }
//       printf("^\n");
//
//     }
#   define pe() return -1;
#   define ni() printf("ERROR: Not implemented yet\n"); return -1;
#   define _printf(...) printf(__VA_ARGS__)
// #   define printf(...) printf("[%s,%d] ", __FILE__, __LINE__); printf(__VA_ARGS__)
#   define msg(...) printf(__VA_ARGS__)
#else
#   define pe() return -1;
#   define ni() return -1;
#   ifdef PRINT_ITERATION_COUNT
#      include <stdio.h>
#      define msg(...) printf(__VA_ARGS__)
#   else
#      define msg(...) ;
#   endif
#endif


int8_t blockrun(State * s){
  if (!s->Mem) {
    s->Mem = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
    memset(s->Mem, 0, sizeof(uint8_t) * MEM_SIZE);
    s->mem_begin = s->Mem;
    s->Ans = 0;
    s->Type = 0;
    s->A.i16[0] = 0;
    s->A.i16[1] = 0;
    s->B.i16[0] = 0;
    s->B.i16[1] = 0;
    s->prev = 0;
    s->lookahead = 0;
    #if defined(MAX_ITERATION_COUNT) || defined(PRINT_ITERATION_COUNT)
      s->_ic = 0;
    #endif
    s->stack = 1;
    s->inc = 1;
    s->r = -1;
  }
  uint8_t len = strlen((char*) s->block);
  uint8_t code = *(s->block);
  if (s->r > 0){
    s->block += len-1;
    code = *(s->block);
    while (code) {
      // printf("0--- %c %d\n", code, stack);
      // printf("---- %c %d\n", code, stack);
      if (code == IF){
        s->stack++;
      } else if (code == ELSE && s->r == JM_EIFE && s->stack == 1) {
        break;
      } else if (code == ENDIF) {
        s->stack--;
        if (s->r <= JM_ENIF && ! s->stack){
          break;
        }
      } else if (code == WHILE) {
        s->stack++;
        if (s->r >= JM_WHI0 && ! s->stack){
          break;
        }
      } else if (code == ENDWHILE) {
        s->stack--;
        if (s->r == JM_EWHI && ! s->stack){
          break;
        }
      }
      s->block += s->inc;
      code = *(s->block);
      if (!code){
        return s->stack;
      }
    }
  }
  while (1){
    if (!code){
      return 0; //EOF
    }
    s->r = step(*(s->block), s);

    #ifdef DEBUG_INTERPRETER
      uint8_t at = s->Mem - s->mem_begin;
      for (uint8_t i = 0; i <= at + 1; i++){
        if (i == at) {
          _printf("[%d|%d> ", s->A.i32, s->B.i32);
        }
        _printf("[%d] ", (s->mem_begin)[i]);
      }
      _printf("\t\t- %c \n", code);
    #endif

    if (s->r < 0){
      return s->r;
    } else if (s->r == 0) {
      s->block++;
      code = *(s->block);
    } else {
      while (code) {
        // printf("0--- %c %d\n", code, stack);
        s->block += s->inc;
        code = *(s->block);
        // printf("---- %c %d\n", code, stack);
        if (code == IF){
          s->stack++;
        } else if (code == ELSE && s->r == JM_EIFE && s->stack == 1) {
          break;
        } else if (code == ENDIF) {
          s->stack--;
          if (s->r <= JM_ENIF && ! s->stack){
            break;
          }
        } else if (code == WHILE) {
          s->stack++;
          if (s->r >= JM_WHI0 && ! s->stack){
            break;
          }
        } else if (code == ENDWHILE) {
          s->stack--;
          if (s->r == JM_EWHI && ! s->stack){
            break;
          }
        }
        if (!code){
          return s->stack;
        }
      }
      if (s->r != JM_WHI0){
        s->block++;
        code = *(s->block);
      }
    }
  }
  return 0;
}

int8_t step(uint8_t code, State * s){
  uint8_t prev = s->prev;
  #ifdef MAX_ITERATION_COUNT
    if (s->_ic > MAX_ITERATION_COUNT) {
      return -1;
    }
  #endif
  
  #ifdef PRINT_ITERATION_COUNT
    if (s->_ic % PRINT_ITERATION_COUNT == 0) {
      msg("%d\n", s->_ic);
    }
  #endif

  #if defined(MAX_ITERATION_COUNT) || defined(PRINT_ITERATION_COUNT)
    s->_ic++;
  #endif
  if (s->lookahead){
    s->lookahead = 0;
    switch (prev) {
      case COND_MODIFIER:
        switch (code) {
          case C_EQ:
            if (s->Type == INT8) s->Ans = s->A.i8[0] == s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] == s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 == s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 == s->B.f32;
            break;
          case C_NEQ:
            if (s->Type == INT8) s->Ans = s->A.i8[0] != s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] != s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 != s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 != s->B.f32;
            break;
          case C_LT:
            if (s->Type == INT8) s->Ans = s->A.i8[0] < s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] < s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 < s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 < s->B.f32;
            break;
          case C_GT:
            if (s->Type == INT8) s->Ans = s->A.i8[0] > s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] > s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 > s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 > s->B.f32;
            break;
          case C_LE:
            if (s->Type == INT8) s->Ans = s->A.i8[0] <= s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] <= s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 <= s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 <= s->B.f32;
            break;
          case C_GE:
            if (s->Type == INT8) s->Ans = s->A.i8[0] >= s->B.i8[0];
            else if (s->Type == INT16) s->Ans = s->A.i16[0] >= s->B.i16[0];
            else if (s->Type == INT32) s->Ans = s->A.i32 >= s->B.i32;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 >= s->B.f32;
            break;
          case C_NOT_NULL:
            if (s->Type == INT8) s->Ans = s->A.i8[0] != 0;
            else if (s->Type == INT16) s->Ans = s->A.i16[0] != 0;
            else if (s->Type == INT32) s->Ans = s->A.i32 != 0;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 != 0;
            break;
          case C_ZERO:
            if (s->Type == INT8) s->Ans = s->A.i8[0] == 0;
            else if (s->Type == INT16) s->Ans = s->A.i16[0] == 0;
            else if (s->Type == INT32) s->Ans = s->A.i32 == 0;
            else if (s->Type == FLOAT) s->Ans = s->A.f32 == 0;
            break;
          default:
            pe();
        }
        return 0;
    }
  }

  switch (code) {
    case COND_MODIFIER:
      s->lookahead = 1;
      break;
    case LEFT:
      if (prev == SHIFT_LEFT){
        if (s->Type == INT8) s->A.i8[0] <<= s->B.i8[0];
        else if (s->Type == INT16) s->A.i16[0] <<= s->B.i16[0];
        else if (s->Type == INT32) s->A.i32 <<= s->B.i32;
        return 0;
      } else {
        uint8_t mod = (prev > '0' && prev < '9') ? s->A.i8[0] : 1;
        if (s->Type == INT8) s->Mem -= 1 * mod;
        else if (s->Type == INT16) s->Mem -= 2 * mod;
        else if (s->Type == INT32) s->Mem -= 4 * mod;
        else if (s->Type == FLOAT) s->Mem -= 4 * mod;
        break;
      }
    case RIGHT:
      if (prev == SHIFT_RIGHT){
        if (s->Type == INT8) s->A.i8[0] >>= s->B.i8[0];
        else if (s->Type == INT16) s->A.i16[0] >>= s->B.i16[0];
        else if (s->Type == INT32) s->A.i32 >>= s->B.i32;
        return 0;
      } else {
        uint8_t mod = (prev > '0' && prev < '9') ? s->A.i8[0] : 1;
        if (s->Type == INT8) s->Mem += 1 * mod;
        else if (s->Type == INT16) s->Mem += 2 * mod;
        else if (s->Type == INT32) s->Mem += 4 * mod;
        else if (s->Type == FLOAT) s->Mem += 4 * mod;
        break;
      }
    case START_STACK:
      s->StackStart = s->Mem;
      break;
    case STACK_HEIGHT:
      s->A.i16[0] = (s->Mem - s->StackStart);
      s->A.i16[1] = 0;
      break;
    case PUSH:
      step(STORE, s);
      step(RIGHT, s);
      break;
    case POP:
      step(LEFT, s);
      step(LOAD, s);
      break;
    case IF:
    case ELSE:
    case WHILE:
    case BREAK:
    case CONTINUE:
    case ENDWHILE:
      {
        uint8_t jmpmatch = JM_NONE;
        s->stack = 1;
        s->inc = 1;
        if (code == IF && !s->Ans){
          // jump to else : + 1 or endif ) + 1
          jmpmatch = JM_EIFE;
        } else if (code == ELSE){
          //jump to endif ) + 1
          jmpmatch = JM_ENIF;
        } else if (code == WHILE && !s->Ans){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (code == BREAK){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (code == CONTINUE){
          //jump to while [
          s->inc = -1;
          s->stack = -1;
          jmpmatch = JM_WHI0;
        } else if (code == ENDWHILE && s->Ans){
          //jump to WHILE [ + 1
          s->inc = -1;
          s->stack = -1;
          jmpmatch = JM_WHI1;
        }
        // printf("%c %d??\n", prev, jmpmatch);
        s->prev = code;
        return jmpmatch;
      }
    case SUM:
      if (s->Type == INT8) s->A.i8[0] += s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] += s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 += s->B.i32;
      else if (s->Type == FLOAT) s->A.f32 += s->B.f32;
      break;
    case SUB:
      if (s->Type == INT8) s->A.i8[0] -= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] -= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 -= s->B.i32;
      else if (s->Type == FLOAT) s->A.f32 -= s->B.f32;
      break;
    case MULTI:
      if (s->Type == INT8) s->A.i8[0] *= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] *= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 *= s->B.i32;
      else if (s->Type == FLOAT) s->A.f32 *= s->B.f32;
      break;
    case DIV:
      if (s->Type == INT8) s->A.i8[0] /= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] /= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 /= s->B.i32;
      else if (s->Type == FLOAT) s->A.f32 /= s->B.f32;
      break;
    case MOD:
      if (s->Type == INT8) s->A.i8[0] %= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] %= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 %= s->B.i32;
      break;
    case AND:
      if (s->Type == INT8) s->A.i8[0] &= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] &= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 &= s->B.i32;
      break;
    case OR:
      if (s->Type == INT8) s->A.i8[0] |= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] |= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 |= s->B.i32;
      break;
    case XOR:
      if (s->Type == INT8) s->A.i8[0] ^= s->B.i8[0];
      else if (s->Type == INT16) s->A.i16[0] ^= s->B.i16[0];
      else if (s->Type == INT32) s->A.i32 ^= s->B.i32;
      break;
    case INV:
      if (s->Type == INT8) s->A.i8[0] = ~s->A.i8[0];
      else if (s->Type == INT16) s->A.i16[0] = ~s->A.i16[0];
      else if (s->Type == INT32) s->A.i32 = ~s->A.i32;
      break;
    case ZERO:
      s->A.i16[0] = 0;
      s->A.i16[1] = 0;
      s->B.i16[0] = 0;
      s->B.i16[1] = 0;
      break;
    case SWITCH:
      s->A.i8[0] ^= s->B.i8[0];
      s->B.i8[0] ^= s->A.i8[0];
      s->A.i8[0] ^= s->B.i8[0];
      s->A.i8[1] ^= s->B.i8[1];
      s->B.i8[1] ^= s->A.i8[1];
      s->A.i8[1] ^= s->B.i8[1];
      s->A.i8[2] ^= s->B.i8[2];
      s->B.i8[2] ^= s->A.i8[2];
      s->A.i8[2] ^= s->B.i8[2];
      s->A.i8[3] ^= s->B.i8[3];
      s->B.i8[3] ^= s->A.i8[3];
      s->A.i8[3] ^= s->B.i8[3];
      break;
    case STORE:
      if (s->Type == INT8) *((uint8_t*)(s->Mem)) = s->A.i8[0];
      else if (s->Type == INT16) *((uint16_t*)(s->Mem)) = s->A.i16[0];
      else if (s->Type == INT32) *((uint32_t*)(s->Mem)) = s->A.i32;
      else if (s->Type == FLOAT) *((float*)(s->Mem)) = s->A.f32;
      break;
    case LOAD:
      if (s->Type == INT8) s->A.i8[0] = *((uint8_t*)(s->Mem));
      else if (s->Type == INT16) s->A.i16[0] = *((uint16_t*)(s->Mem));
      else if (s->Type == INT32) s->A.i32 = *((uint32_t*)(s->Mem));
      else if (s->Type == FLOAT) s->A.f32 = *((float*)(s->Mem));
      break;
    case T_INT8:
      s->Type = 0;
      break;
    case T_INT16:
      s->Type = 1;
      break;
    case T_INT32:
      s->Type = 2;
      break;
    case T_FLOAT:
      s->Type = 3;
      break;
    case IN:
    case OUT:
      if (prev >= 'A' && prev <= 'Z'){
        api(prev, code, s);
      } else {
        api(0, code, s);
      }
      break;
    case ENDIF:
    case NOP:
      break;

    default:
      if (code >= '0' && code <= '9'){
        if (!prev || ((prev < '0' || prev > '9') && prev != T_INT8 && prev != T_INT16 && prev != T_INT32 && prev != T_FLOAT)){
          if (s->Type == FLOAT) {
            s->A.f32 = (code - '0');
          } else {
            s->A.i16[0] = (code - '0');
            s->A.i16[1] = 0;
          }
        } else {
          if (s->Type == INT8) s->A.i8[0] = s->A.i8[0]*10 + (code - '0');
          else if (s->Type == INT16) s->A.i16[0] = s->A.i16[0]*10 + (code - '0');
          else if (s->Type == INT32) s->A.i32 = s->A.i32*10 + (code - '0');
          else if (s->Type == FLOAT) s->A.f32 = s->A.f32*10 + (code - '0');
        }
      } else {
        pe();
      }
      break;
  }
  s->prev = code;
  return 0;
}
