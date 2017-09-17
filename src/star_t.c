#include <stdlib.h>
#include <star_t.h>

#ifdef DEBUG_INTERPRETER
#   include <stdio.h>
#   include <microcuts.h>
#   define pe() __parsing_error(__FILE__, __LINE__, (const char*) code_begin, (const char*) code); return -1;
    void __parsing_error(const char* file, int line, const char* code_begin, const char* pos){
      int at = (int)(pos - code_begin);
      char token = *(pos);
      printf("Parsing error: Unexpected token '%c' at position %d on file '%s', line %d.\n", token, at, file, line);
      printf("\t%s\n\t", code_begin);
      while(at){
        at--;
        printf(" ");
      }
      printf("^\n");

    }
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
State * s;

void begin(){
  if (mem_begin){
    free(mem_begin);
  }
  mem = mem_begin = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
  code_begin = NULL;
  s = (State*) malloc(sizeof(State));
  s->mem = mem;
  s->info.comp = 0;
  s->info.type = 0;
  s->A.i16[0] = 0;
  s->A.i16[1] = 0;
  s->B.i16[0] = 0;
  s->B.i16[1] = 0;
}

int8_t run(uint8_t* code){
  begin();
  #if defined(MAX_ITERATION_COUNT) || defined(PRINT_ITERATION_COUNT)
    _ic = 0;
  #endif
  code_begin = code;
  while (*code){
    #ifdef MAX_ITERATION_COUNT
      if (_ic >= MAX_ITERATION_COUNT) { msg("ITERATION OVERFLOW\n"); break;} else _ic++;
    #elif defined(PRINT_ITERATION_COUNT)
      _ic++;
    #endif
    switch (*code) {
      case COND_MODIFIER:
        switch (next(1)) {
          case C_EQ:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] == s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] == s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 == s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 == s->B.f32;
            break;
          case C_NEQ:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] != s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] != s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 != s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 != s->B.f32;
            break;
          case C_LT:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] < s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] < s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 < s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 < s->B.f32;
            break;
          case C_GT:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] > s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] > s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 > s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 > s->B.f32;
            break;
          case C_LE:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] <= s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] <= s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 <= s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 <= s->B.f32;
            break;
          case C_GE:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] >= s->B.i8[0];
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] >= s->B.i16[0];
            else if (s->info.type == INT32) s->info.comp = s->A.i32 >= s->B.i32;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 >= s->B.f32;
            break;
          case C_NOT_NULL:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] != 0;
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] != 0;
            else if (s->info.type == INT32) s->info.comp = s->A.i32 != 0;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 != 0;
            break;
          case C_ZERO:
            if (s->info.type == INT8) s->info.comp = s->A.i8[0] == 0;
            else if (s->info.type == INT16) s->info.comp = s->A.i16[0] == 0;
            else if (s->info.type == INT32) s->info.comp = s->A.i32 == 0;
            else if (s->info.type == FLOAT) s->info.comp = s->A.f32 == 0;
            break;
          default:
            if (next(1) >= 'A' && next(1) <= 'Z'){
              consume();
              continue;
            }
            pe();
        }
        consume();
        break;
      case TYPE_SET:
        switch (next(1)) {
          case T_INT8:
            s->info.type = 0;
            break;
          case T_INT16:
            s->info.type = 1;
            break;
          case T_INT32:
            s->info.type = 2;
            break;
          case T_FLOAT:
            s->info.type = 3;
            break;
          default:
            pe();
        }
        break;
      case LEFT:
        if (next(1) == SHIFT_LEFT){
          if (s->info.type == INT8) s->A.i8[0] <<= s->B.i8[0];
          else if (s->info.type == INT16) s->A.i16[0] <<= s->B.i16[0];
          else if (s->info.type == INT32) s->A.i32 <<= s->B.i32;
          consume();
        } else {
          uint8_t inc = ((code - code_begin) && prev(1) > '0' && prev(1) < '9') ? s->A.i8[0] : 1;
          if (s->info.type == INT8) s->mem -= 1 * inc;
          else if (s->info.type == INT16) s->mem -= 2 * inc;
          else if (s->info.type == INT32) s->mem -= 4 * inc;
          else if (s->info.type == FLOAT) s->mem -= 4 * inc;
        }
        break;
      case RIGHT:
        if (next(1) == SHIFT_RIGHT){
          if (s->info.type == INT8) s->A.i8[0] >>= s->B.i8[0];
          else if (s->info.type == INT16) s->A.i16[0] >>= s->B.i16[0];
          else if (s->info.type == INT32) s->A.i32 >>= s->B.i32;
          consume();
        } else {
          uint8_t inc = ((code - code_begin) && prev(1) > '0' && prev(1) < '9') ? s->A.i8[0] : 1;
          if (s->info.type == INT8) s->mem += 1 * inc;
          else if (s->info.type == INT16) s->mem += 2 * inc;
          else if (s->info.type == INT32) s->mem += 4 * inc;
          else if (s->info.type == FLOAT) s->mem += 4 * inc;
        }
        break;
      case SUM:
        if (s->info.type == INT8) s->A.i8[0] += s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] += s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 += s->B.i32;
        else if (s->info.type == FLOAT) s->A.f32 += s->B.f32;
        break;
      case SUB:
        if (s->info.type == INT8) s->A.i8[0] -= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] -= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 -= s->B.i32;
        else if (s->info.type == FLOAT) s->A.f32 -= s->B.f32;
        break;
      case MULTI:
        if (s->info.type == INT8) s->A.i8[0] *= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] *= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 *= s->B.i32;
        else if (s->info.type == FLOAT) s->A.f32 *= s->B.f32;
        break;
      case DIV:
        if (s->info.type == INT8) s->A.i8[0] /= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] /= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 /= s->B.i32;
        else if (s->info.type == FLOAT) s->A.f32 /= s->B.f32;
        break;
      case MOD:
        if (s->info.type == INT8) s->A.i8[0] %= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] %= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 %= s->B.i32;
        break;
      case AND:
        if (s->info.type == INT8) s->A.i8[0] &= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] &= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 &= s->B.i32;
        break;
      case OR:
        if (s->info.type == INT8) s->A.i8[0] |= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] |= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 |= s->B.i32;
        break;
      case XOR:
        if (s->info.type == INT8) s->A.i8[0] ^= s->B.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] ^= s->B.i16[0];
        else if (s->info.type == INT32) s->A.i32 ^= s->B.i32;
        break;
      case INV:
        if (s->info.type == INT8) s->A.i8[0] = ~s->A.i8[0];
        else if (s->info.type == INT16) s->A.i16[0] = ~s->A.i16[0];
        else if (s->info.type == INT32) s->A.i32 = ~s->A.i32;
        break;
      case CLEAR:
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
        if (s->info.type == INT8) *((byte_t*)(s->mem)) = s->A.i8[0];
        else if (s->info.type == INT16) *((int_t*)(s->mem)) = s->A.i16[0];
        else if (s->info.type == INT32) *((long_t*)(s->mem)) = s->A.i32;
        else if (s->info.type == FLOAT) *((float*)(s->mem)) = s->A.f32;
        break;
      case LOAD:
        if (s->info.type == INT8) s->A.i8[0] = *((byte_t*)(s->mem));
        else if (s->info.type == INT16) s->A.i16[0] = *((int_t*)(s->mem));
        else if (s->info.type == INT32) s->A.i32 = *((long_t*)(s->mem));
        else if (s->info.type == FLOAT) s->A.f32 = *((float*)(s->mem));
        break;
      case T_INT8:
        s->info.type = 0;
        s->A.i16[0] = 0;
        s->A.i16[1] = 0;
        break;
      case T_INT16:
        s->info.type = 1;
        s->A.i16[0] = 0;
        s->A.i16[1] = 0;
        break;
      case T_INT32:
        s->info.type = 2;
        s->A.i32 = 0;
        break;
      case T_FLOAT:
        s->info.type = 3;
        s->A.f32 = 0;
        break;
      case IN:
      case OUT:
        if (next(1) >= 'A' && next(1) <= 'Z'){
          break;
        } else {
            api(0, *code, s);
        }
      case ENDIF:
      case NOP:
        break;
      case IF:
      case ELSE:
      case WHILE:
      case BREAK:
      case CONTINUE:
      case ENDWHILE:
      {
        // printf("%c %d\n", *code, s->info.comp);
        uint8_t jmpmatch = JM_NONE;
        int8_t stack = 1;
        int8_t inc = 1;
        if (*code == IF && !s->info.comp){
          // jump to else ){ + 1 or endif ) + 1
          jmpmatch = JM_EIFE;
        } else if (*code == ELSE){
          //jump to endif ) + 1
          jmpmatch = JM_ENIF;
        } else if (*code == WHILE && !s->info.comp){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (*code == BREAK){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_EWHI;
        } else if (*code == CONTINUE){
          //jump to while [
          inc = -1;
          stack = -1;
          jmpmatch = JM_WHI0;
        } else if (*code == ENDWHILE && s->info.comp){
          //jump to WHILE [ + 1
          inc = -1;
          stack = -1;
          jmpmatch = JM_WHI1;
        }
        if (jmpmatch != JM_NONE){
          // printf("%c %d\n", *code, jmpmatch);
          while (*code) {
            code += inc;
            // printf("---- %c %d\n", *code, stack);
            if (*code == IF){
              stack++;
            } else if (*code == ELSE && jmpmatch == JM_EIFE && stack == 1) {
              break;
            } else if (*code == ENDIF) {
              stack--;
              if (jmpmatch <= JM_ENIF && !stack){
                break;
              }
            } else if (*code == WHILE) {
              stack++;
              if (jmpmatch >= JM_WHI0 && !stack){
                break;
              }
            } else if (*code == ENDWHILE) {
              stack--;
              if (jmpmatch == JM_EWHI && !stack){
                break;
              }
            }
            if (!*code){
              return stack;
            }
          }
          if (jmpmatch == JM_WHI0){
            code--;
          }
        }
        break;
      }
      default:
        if (*code >= '0' && *code <= '9'){
          if (!(code - code_begin) || ((prev(1) < '0' || prev(1) > '9') && prev(1) != T_INT8 && prev(1) != T_INT16 && prev(1) != T_INT32 && prev(1) != T_FLOAT)){
            s->A.i16[0] = (*code - '0');
            s->A.i16[1] = 0;
          } else {
            if (s->info.type == INT8) s->A.i8[0] = s->A.i8[0]*10 + (*code - '0');
            else if (s->info.type == INT16) s->A.i16[0] = s->A.i16[0]*10 + (*code - '0');
            else if (s->info.type == INT32) s->A.i32 = s->A.i32*10 + (*code - '0');
            else if (s->info.type == FLOAT) { ni(); }
          }
        } else if (*code >= 'A' && *code <= 'Z'){
          api(prev(1), *code, s);
        } else {
          pe();
        }
        break;
    }
    consume();
  }
  #ifdef PRINT_ITERATION_COUNT
    if (_ic >= PRINT_ITERATION_COUNT) { msg("ITERATION COUNTER: %d\n", _ic); }
  #endif
  return 0;
}
