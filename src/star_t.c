#include <stdlib.h>
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
#   define printf(...) printf("[%s,%d] ", __FILE__, __LINE__); printf(__VA_ARGS__)
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

#define next(x) *(code + x)
#define prev(x) *(code - x)
#define consume() (code++)

#if defined(MAX_ITERATION_COUNT) || defined(PRINT_ITERATION_COUNT)
  uint16_t _ic = 0;
#endif
uint8_t* mem = NULL;
uint8_t* mem_begin = NULL;
uint8_t* code_begin = NULL;
uint8_t prev;
uint8_t lookahead;
int8_t stack = 1;
int8_t inc = 1;
State * s;

void begin(){
  if (mem_begin){
    free(mem_begin);
  }
  mem = mem_begin = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
  code_begin = NULL;
  if (!s){
    s = (State*) malloc(sizeof(State));
  }
  s->Mem = mem;
  s->Ans = 0;
  s->Type = 0;
  s->A.i16[0] = 0;
  s->A.i16[1] = 0;
  s->B.i16[0] = 0;
  s->B.i16[1] = 0;
  prev = 0;
  lookahead = 0;
}

int8_t r = -1;
int8_t blockrun(uint8_t* block, uint8_t len){
  if (r > 0){
    block += len-1;
    while (*block) {
      // printf("0--- %c %d\n", *block, stack);
      // printf("---- %c %d\n", *block, stack);
      if (*block == IF){
        stack++;
      } else if (*block == ELSE && r == JM_EIFE && stack == 1) {
        break;
      } else if (*block == ENDIF) {
        stack--;
        if (r <= JM_ENIF && !stack){
          break;
        }
      } else if (*block == WHILE) {
        stack++;
        if (r >= JM_WHI0 && !stack){
          break;
        }
      } else if (*block == ENDWHILE) {
        stack--;
        if (r == JM_EWHI && !stack){
          break;
        }
      }
      block += inc;
      if (!*block){
        return stack;
      }
    }
  }
  while (1){
    if (!*block){
      return 0; //EOF
    }
    r = step(*block);
    if (r < 0){
      return r;
    } else if (r == 0) {
      block++;
    } else {
      while (*block) {
        // printf("0--- %c %d\n", *block, stack);
        block += inc;
        // printf("---- %c %d\n", *block, stack);
        if (*block == IF){
          stack++;
        } else if (*block == ELSE && r == JM_EIFE && stack == 1) {
          break;
        } else if (*block == ENDIF) {
          stack--;
          if (r <= JM_ENIF && !stack){
            break;
          }
        } else if (*block == WHILE) {
          stack++;
          if (r >= JM_WHI0 && !stack){
            break;
          }
        } else if (*block == ENDWHILE) {
          stack--;
          if (r == JM_EWHI && !stack){
            break;
          }
        }
        if (!*block){
          return stack;
        }
      }
      if (r != JM_WHI0){
        block++;
      }
    }
  }
  return 0;
}

int8_t runall(uint8_t* code){
  begin();
  int8_t r;
  while (*code){
    r = step(*code);
    if (r < 0){
      return r;
    } else if (r == 0) {
      code++;
    } else {
      // printf("%c %d\n", *code, r);
      while (*code) {
        code += inc;
        // printf("---- %c %d\n", *code, stack);
        if (*code == IF){
          stack++;
        } else if (*code == ELSE && r == JM_EIFE && stack == 1) {
          break;
        } else if (*code == ENDIF) {
          stack--;
          if (r <= JM_ENIF && !stack){
            break;
          }
        } else if (*code == WHILE) {
          stack++;
          if (r >= JM_WHI0 && !stack){
            break;
          }
        } else if (*code == ENDWHILE) {
          stack--;
          if (r == JM_EWHI && !stack){
            break;
          }
        }
        if (!*code){
          return stack;
        }
      }
      if (r != JM_WHI0){
        code++;
      }
    }
  }
  return 0;
}

int8_t step(uint8_t code){
  if (lookahead){
    lookahead = 0;
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
      lookahead = 1;
      break;
    case LEFT:
      if (prev == SHIFT_LEFT){
        if (s->Type == INT8) s->A.i8[0] <<= s->B.i8[0];
        else if (s->Type == INT16) s->A.i16[0] <<= s->B.i16[0];
        else if (s->Type == INT32) s->A.i32 <<= s->B.i32;
        return 0;
      } else {
        uint8_t inc = (prev > '0' && prev < '9') ? s->A.i8[0] : 1;
        if (s->Type == INT8) s->Mem -= 1 * inc;
        else if (s->Type == INT16) s->Mem -= 2 * inc;
        else if (s->Type == INT32) s->Mem -= 4 * inc;
        else if (s->Type == FLOAT) s->Mem -= 4 * inc;
        break;
      }
    case RIGHT:
      if (prev == SHIFT_RIGHT){
        if (s->Type == INT8) s->A.i8[0] >>= s->B.i8[0];
        else if (s->Type == INT16) s->A.i16[0] >>= s->B.i16[0];
        else if (s->Type == INT32) s->A.i32 >>= s->B.i32;
        return 0;
      } else {
        uint8_t inc = (prev > '0' && prev < '9') ? s->A.i8[0] : 1;
        if (s->Type == INT8) s->Mem += 1 * inc;
        else if (s->Type == INT16) s->Mem += 2 * inc;
        else if (s->Type == INT32) s->Mem += 4 * inc;
        else if (s->Type == FLOAT) s->Mem += 4 * inc;
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
      step(STORE);
      step(RIGHT);
      break;
    case POP:
      step(LEFT);
      step(LOAD);
      break;
    case IF:
    case ELSE:
    case WHILE:
    case BREAK:
    case CONTINUE:
    case ENDWHILE:
      {
        uint8_t jmpmatch = JM_NONE;
        stack = 1;
        inc = 1;
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
          inc = -1;
          stack = -1;
          jmpmatch = JM_WHI0;
        } else if (code == ENDWHILE && s->Ans){
          //jump to WHILE [ + 1
          inc = -1;
          stack = -1;
          jmpmatch = JM_WHI1;
        }
        // printf("%c %d??\n", prev, jmpmatch);
        prev = code;
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
          s->A.i16[0] = (code - '0');
          s->A.i16[1] = 0;
        } else {
          if (s->Type == INT8) s->A.i8[0] = s->A.i8[0]*10 + (code - '0');
          else if (s->Type == INT16) s->A.i16[0] = s->A.i16[0]*10 + (code - '0');
          else if (s->Type == INT32) s->A.i32 = s->A.i32*10 + (code - '0');
          else if (s->Type == FLOAT) { pe(); }
        }
      } else {
        pe();
      }
      break;
  }
  prev = code;
  return 0;
}
