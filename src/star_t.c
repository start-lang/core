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
  if (!s->_m) {
    s->_m = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
    memset(s->_m, 0, sizeof(uint8_t) * MEM_SIZE);
    s->_m0 = s->_m;
    s->_src0 = s->src;
    s->_matching = 1;
    s->_forward = 1;
    s->_prev_step_result = JM_ERR0;
  }
  uint8_t len = strlen((char*) s->src);
  uint8_t token = *(s->src);
  if (s->_prev_step_result > 0 && s->_prev_step_result < JM_ERR0){
    s->src += len-1;
    token = *(s->src);
    while (token) {
      if (token == IF){
        s->_matching++;
      } else if (token == ELSE && s->_prev_step_result == JM_EIFE && s->_matching == 1) {
        break;
      } else if (token == ENDIF) {
        s->_matching--;
        if (s->_prev_step_result <= JM_ENIF && ! s->_matching){
          break;
        }
      } else if (token == WHILE) {
        s->_matching++;
        if (s->_prev_step_result >= JM_WHI0 && ! s->_matching){
          break;
        }
      } else if (token == ENDWHILE) {
        s->_matching--;
        if (s->_prev_step_result == JM_EWHI && ! s->_matching){
          break;
        }
      }
      if (s->src - s->_src0 == 0 && ! s->_forward) {
        return -1;
      }
      s->src += s->_forward ? 1 : -1;
      token = *(s->src);
      if (!token){
        return s->_matching;
      }
    }
  }
  while (1){
    if (!token){
      return 0; //EOF
    }
    s->_prev_step_result = step(*(s->src), s);

    #ifdef DEBUG_INTERPRETER
      uint8_t at = s->_m - s->_m0;
      for (uint8_t i = 0; i <= at + 1; i++){
        if (i == at) {
          _printf("[%d|%d> ", s->a.i32, s->b.i32);
        }
        _printf("[%d] ", (s->_m0)[i]);
      }
      _printf("\t\t- %c \n", token);
    #endif

    if (s->_prev_step_result >= JM_ERR0){
      return s->_prev_step_result;
    } else if (s->_prev_step_result == 0) {
      s->src++;
      token = *(s->src);
    } else {
      while (token) {
        if (s->src - s->_src0 == 0 && ! s->_forward) {
          return -1;
        }
        s->src += s->_forward ? 1 : -1;;
        token = *(s->src);
        if (token == IF){
          s->_matching++;
        } else if (token == ELSE && s->_prev_step_result == JM_EIFE && s->_matching == 1) {
          break;
        } else if (token == ENDIF) {
          s->_matching--;
          if (s->_prev_step_result <= JM_ENIF && ! s->_matching){
            break;
          }
        } else if (token == WHILE) {
          s->_matching++;
          if (s->_prev_step_result >= JM_WHI0 && ! s->_matching){
            break;
          }
        } else if (token == ENDWHILE) {
          s->_matching--;
          if (s->_prev_step_result == JM_EWHI && ! s->_matching){
            break;
          }
        }
        if (!token){
          return s->_matching;
        }
      }
      if (s->_prev_step_result != JM_WHI0){
        s->src++;
        token = *(s->src);
      }
    }
  }
  return 0;
}

uint8_t step(uint8_t token, State * s){
  uint8_t prev = s->_prev_token;
  s->_ic++;
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

  if (s->_lookahead){
    s->_lookahead = 0;
    switch (prev) {
      case COND_MODIFIER:
        switch (token) {
          case C_EQ:
            if (s->_type == INT8) s->_ans = s->a.i8[0] == s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] == s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 == s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 == s->b.f32;
            break;
          case C_NEQ:
            if (s->_type == INT8) s->_ans = s->a.i8[0] != s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] != s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 != s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 != s->b.f32;
            break;
          case C_LT:
            if (s->_type == INT8) s->_ans = s->a.i8[0] < s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] < s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 < s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 < s->b.f32;
            break;
          case C_GT:
            if (s->_type == INT8) s->_ans = s->a.i8[0] > s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] > s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 > s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 > s->b.f32;
            break;
          case C_LE:
            if (s->_type == INT8) s->_ans = s->a.i8[0] <= s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] <= s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 <= s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 <= s->b.f32;
            break;
          case C_GE:
            if (s->_type == INT8) s->_ans = s->a.i8[0] >= s->b.i8[0];
            else if (s->_type == INT16) s->_ans = s->a.i16[0] >= s->b.i16[0];
            else if (s->_type == INT32) s->_ans = s->a.i32 >= s->b.i32;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 >= s->b.f32;
            break;
          case C_NOT_NULL:
            if (s->_type == INT8) s->_ans = s->a.i8[0] != 0;
            else if (s->_type == INT16) s->_ans = s->a.i16[0] != 0;
            else if (s->_type == INT32) s->_ans = s->a.i32 != 0;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 != 0;
            break;
          case C_ZERO:
            if (s->_type == INT8) s->_ans = s->a.i8[0] == 0;
            else if (s->_type == INT16) s->_ans = s->a.i16[0] == 0;
            else if (s->_type == INT32) s->_ans = s->a.i32 == 0;
            else if (s->_type == FLOAT) s->_ans = s->a.f32 == 0;
            break;
          default:
            return JM_PEXC;
        }
        return 0;
    }
  }

  switch (token) {
    case COND_MODIFIER:
      s->_lookahead = 1;
      break;
    case LEFT:
      if (prev == SHIFT_LEFT){
        if (s->_type == INT8) s->a.i8[0] <<= s->b.i8[0];
        else if (s->_type == INT16) s->a.i16[0] <<= s->b.i16[0];
        else if (s->_type == INT32) s->a.i32 <<= s->b.i32;
        return 0;
      } else {
        uint8_t mod = (prev > '0' && prev < '9') ? s->a.i8[0] : 1;
        if (s->_type == INT8) s->_m -= 1 * mod;
        else if (s->_type == INT16) s->_m -= 2 * mod;
        else if (s->_type == INT32) s->_m -= 4 * mod;
        else if (s->_type == FLOAT) s->_m -= 4 * mod;
        break;
      }
    case RIGHT:
      if (prev == SHIFT_RIGHT){
        if (s->_type == INT8) s->a.i8[0] >>= s->b.i8[0];
        else if (s->_type == INT16) s->a.i16[0] >>= s->b.i16[0];
        else if (s->_type == INT32) s->a.i32 >>= s->b.i32;
        return 0;
      } else {
        uint8_t mod = (prev > '0' && prev < '9') ? s->a.i8[0] : 1;
        if (s->_type == INT8) s->_m += 1 * mod;
        else if (s->_type == INT16) s->_m += 2 * mod;
        else if (s->_type == INT32) s->_m += 4 * mod;
        else if (s->_type == FLOAT) s->_m += 4 * mod;
        break;
      }
    case START_STACK:
      s->v_stack0 = s->_m;
      break;
    case STACK_HEIGHT:
      s->a.i16[0] = (s->_m - s->v_stack0);
      s->a.i16[1] = 0;
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
        s->_matching = 1;
        s->_forward = 1;
        if (token == IF && !s->_ans){
          // jump to else : + 1 or endif ) + 1
          jmpmatch = JM_EIFE;
        } else if (token == ELSE){
          //jump to endif ) + 1
          jmpmatch = JM_ENIF;
        } else if (token == WHILE && !s->_ans){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (token == BREAK){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (token == CONTINUE){
          //jump to while [
          s->_forward = 0;
          s->_matching = -1;
          jmpmatch = JM_WHI0;
        } else if (token == ENDWHILE && s->_ans){
          //jump to WHILE [ + 1
          s->_forward = 0;
          s->_matching = -1;
          jmpmatch = JM_WHI1;
        }
        // printf("%c %d??\n", prev, jmpmatch);
        s->_prev_token = token;
        return jmpmatch;
      }
    case SUM:
      if (s->_type == INT8) s->a.i8[0] += s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] += s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 += s->b.i32;
      else if (s->_type == FLOAT) s->a.f32 += s->b.f32;
      break;
    case SUB:
      if (s->_type == INT8) s->a.i8[0] -= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] -= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 -= s->b.i32;
      else if (s->_type == FLOAT) s->a.f32 -= s->b.f32;
      break;
    case MULTI:
      if (s->_type == INT8) s->a.i8[0] *= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] *= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 *= s->b.i32;
      else if (s->_type == FLOAT) s->a.f32 *= s->b.f32;
      break;
    case DIV:
      if (s->_type == INT8) s->a.i8[0] /= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] /= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 /= s->b.i32;
      else if (s->_type == FLOAT) s->a.f32 /= s->b.f32;
      break;
    case MOD:
      if (s->_type == INT8) s->a.i8[0] %= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] %= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 %= s->b.i32;
      break;
    case AND:
      if (s->_type == INT8) s->a.i8[0] &= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] &= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 &= s->b.i32;
      break;
    case OR:
      if (s->_type == INT8) s->a.i8[0] |= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] |= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 |= s->b.i32;
      break;
    case XOR:
      if (s->_type == INT8) s->a.i8[0] ^= s->b.i8[0];
      else if (s->_type == INT16) s->a.i16[0] ^= s->b.i16[0];
      else if (s->_type == INT32) s->a.i32 ^= s->b.i32;
      break;
    case INV:
      if (s->_type == INT8) s->a.i8[0] = ~s->a.i8[0];
      else if (s->_type == INT16) s->a.i16[0] = ~s->a.i16[0];
      else if (s->_type == INT32) s->a.i32 = ~s->a.i32;
      break;
    case ZERO:
      s->a.i16[0] = 0;
      s->a.i16[1] = 0;
      s->b.i16[0] = 0;
      s->b.i16[1] = 0;
      break;
    case SWITCH:
      s->a.i8[0] ^= s->b.i8[0];
      s->b.i8[0] ^= s->a.i8[0];
      s->a.i8[0] ^= s->b.i8[0];
      s->a.i8[1] ^= s->b.i8[1];
      s->b.i8[1] ^= s->a.i8[1];
      s->a.i8[1] ^= s->b.i8[1];
      s->a.i8[2] ^= s->b.i8[2];
      s->b.i8[2] ^= s->a.i8[2];
      s->a.i8[2] ^= s->b.i8[2];
      s->a.i8[3] ^= s->b.i8[3];
      s->b.i8[3] ^= s->a.i8[3];
      s->a.i8[3] ^= s->b.i8[3];
      break;
    case STORE:
      if (s->_type == INT8) *((uint8_t*)(s->_m)) = s->a.i8[0];
      else if (s->_type == INT16) *((uint16_t*)(s->_m)) = s->a.i16[0];
      else if (s->_type == INT32) *((uint32_t*)(s->_m)) = s->a.i32;
      else if (s->_type == FLOAT) *((float*)(s->_m)) = s->a.f32;
      break;
    case LOAD:
      if (s->_type == INT8) s->a.i8[0] = *((uint8_t*)(s->_m));
      else if (s->_type == INT16) s->a.i16[0] = *((uint16_t*)(s->_m));
      else if (s->_type == INT32) s->a.i32 = *((uint32_t*)(s->_m));
      else if (s->_type == FLOAT) s->a.f32 = *((float*)(s->_m));
      break;
    case T_INT8:
      s->_type = 0;
      break;
    case T_INT16:
      s->_type = 1;
      break;
    case T_INT32:
      s->_type = 2;
      break;
    case T_FLOAT:
      s->_type = 3;
      break;
    case IN:
    case OUT:
      if (prev >= 'A' && prev <= 'Z'){
        api(prev, token, s);
      } else {
        api(0, token, s);
      }
      break;
    case ENDIF:
    case NOP:
      break;

    default:
      if (token >= '0' && token <= '9'){
        if (!prev || ((prev < '0' || prev > '9') && prev != T_INT8 && prev != T_INT16 && prev != T_INT32 && prev != T_FLOAT)){
          if (s->_type == FLOAT) {
            s->a.f32 = (token - '0');
          } else {
            s->a.i16[0] = (token - '0');
            s->a.i16[1] = 0;
          }
        } else {
          if (s->_type == INT8) s->a.i8[0] = s->a.i8[0]*10 + (token - '0');
          else if (s->_type == INT16) s->a.i16[0] = s->a.i16[0]*10 + (token - '0');
          else if (s->_type == INT32) s->a.i32 = s->a.i32*10 + (token - '0');
          else if (s->_type == FLOAT) s->a.f32 = s->a.f32*10 + (token - '0');
        }
      } else {
        return JM_PEXC;
      }
      break;
  }
  s->_prev_token = token;
  return 0;
}
