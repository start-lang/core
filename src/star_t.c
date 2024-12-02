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

Variable * _vars = NULL;
uint8_t _varc = 0;

RTFunction * _funcs = NULL;
uint8_t _funcc = 0;

uint8_t jump(State * s){
  uint8_t token = *(s->src);
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
      if (s->_prev_step_result == JM_NXWH){
        break;
      }
    }
    if (s->src - s->_src0 == 0 && ! s->_forward) {
      return 1;
    }
    s->src += s->_forward ? 1 : -1;
    token = *(s->src);
  }
  return 0;
}

void free_memory(State * s){
  if (s == NULL) return;
  free(s->_m0);
  for (uint8_t i = 0; i < _varc; i++){
    free(_vars[i].name);
  }
  for (uint8_t i = 0; i < _funcc; i++){
    free(_funcs[i].name);
    free(_funcs[i].src);
  }
  if (_vars){
    free(_vars);
    _vars = NULL;
    _varc = 0;
  }
  if (_funcs){
    free(_funcs);
    _funcs = NULL;
    _funcc = 0;
  }
  if (s->_id){
   free(s->_id);
  }
  free(s);
}

int8_t blockrun(State * s){
  if (!s->_m) {
    s->_m = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
    memset(s->_m, 0, sizeof(uint8_t) * MEM_SIZE);
    s->_m0 = s->_m;
    s->_mlen = MEM_SIZE;
    s->_src0 = s->src;
    s->_matching = 1;
    s->_forward = 1;
    s->_prev_step_result = JM_ERR0;
    s->reg.i8[0] = 1;
  }

  while (1){
    if (*(s->src) == 0 && !s->_idlen && !s->_lookahead){
      break;
    }
    if (s->_prev_step_result > 0 && s->_prev_step_result < JM_ERR0){
      if (! s->_forward) {
        s->src += strlen((char*) s->src) - 1;
        if (jump(s)) return -1;
      }
    }

    #ifdef MAX_ITERATION_COUNT
      if (s->_ic > MAX_ITERATION_COUNT) {
        return JM_ERR0;
      }
    #endif

    #ifdef PRINT_ITERATION_COUNT
      if (s->_ic % PRINT_ITERATION_COUNT == 0) {
        msg("%d\n", s->_ic);
      }
    #endif

    s->_prev_step_result = step(*(s->src), s);
    if (step_callback(s) != 0){
      return 0; // TODO change return val?
    }
    if (s->sub){
      blockrun(s->sub);
      if (s->sub->_freesrc){
        free(s->sub->_src0);
      }
      free(s->sub);
      s->sub = NULL;
    }

    if (s->_prev_step_result >= JM_ERR0){
      return s->_prev_step_result;
    } else if (s->_prev_step_result == 0) {
      if (*(s->src) != 0){
        s->src++;
      }
    } else {
      if (s->src - s->_src0 == 0 && ! s->_forward) {
        return -1;
      }
      s->src += s->_forward ? 1 : -1;;
      if (s->_prev_step_result != 0) {
        if (jump(s)) return -1;
        if (s->_prev_step_result != JM_WHI0){
          s->src++;
          s->_prev_step_result = 0;
        }
      }
    }
  }
  return 0;
}

void new_sub(uint8_t * src, State * s) {
  State* sub = (State*) malloc(sizeof(State));
  memset(sub, 0, sizeof(State));
  sub->src = src;
  sub->_m = s->_m;
  sub->_m0 = sub->_m;
  sub->_mlen = s->_mlen - (sub->_m - s->_m);
  sub->_src0 = sub->src;
  sub->_matching = 1;
  sub->_forward = 1;
  sub->_prev_step_result = JM_ERR0;
  s->sub = sub;
}

Register mload(State * s) {
  Register x = {.i32 = 0};
  if (s->_type == INT16) {
    x.i8[0] = s->_m[0];
    x.i8[1] = s->_m[1];
  } else if (s->_type != INT8) {
    x.i8[0] = s->_m[0];
    x.i8[1] = s->_m[1];
    x.i8[2] = s->_m[2];
    x.i8[3] = s->_m[3];
  }
  return x;
}

uint8_t step(uint8_t token, State * s){

  #ifdef DEBUG_INTERPRETER
    for (uint8_t * i = s->_m0; i <= s->_m + 2; i++){
      if (i == s->_m) {
        _printf("[%d:%d:%d> ", REG.i32, s->_ans, s->_type);
      }
      _printf("[%d] ", i[0]);
    }
    _printf("\t\t- %c \n", token);
  #endif

  uint8_t prev = s->_prev_token;
  s->_prev_token = token;
  if (s->_string){
    if (s->_m - s->_m0 >= s->_mlen){
      return JM_REOB;
    }
    if (token != SCAPE) {
      s->_m[0] = token;
      REG.i8[0]++;
      if (token == STRING && prev != SCAPE) {
        s->_m[0] = 0;
        s->_m -= REG.i8[0];
        REG.i8[0]--;
        s->_string = 0;
      }
    } else if (prev == SCAPE) {
      s->_m[0] = token;
      REG.i8[0]++;
    } else {
      return 0;
    }
    s->_m++;
    return 0;
  }

  if (s->_srcinput){
    RTFunction * f = &(_funcs[_funcc - 1]);
    f->src = realloc(f->src, s->_lensrc + 1);
    f->src[s->_lensrc] = token;
    if (token == ENDFUNCTION) {
      s->_fmatching--;
      if (s->_fmatching == 0){
        f->src[s->_lensrc] = 0;
        s->_srcinput = 0;
      }
    } else if (token == STARTFUNCTION) {
      s->_fmatching++;
    }
    s->_lensrc++;
    return 0;
  }

  if (token == PRINT) {
    s->_id = (uint8_t*) malloc(2);
    s->_id[0] = 'P';
    s->_id[1] = 'C';
    s->_idlen = 2;
  } else if (token == INPUT) {
    s->_id = (uint8_t*) malloc(2);
    s->_id[0] = 'I';
    s->_id[1] = 'N';
    s->_idlen = 2;
  }

  if ((token >= 'A' && token <= 'Z') || token == '_'
      || (s->_idlen && (token >= '0' && token <= '9'))){
    s->_id = (uint8_t*) realloc(s->_id, s->_idlen + 1);
    s->_id[s->_idlen] = token;
    s->_idlen++;
    return 0;
  } else if (s->_idlen) {
    s->_id = (uint8_t*) realloc(s->_id, s->_idlen + 1);
    s->_id[s->_idlen] = '\0';
    if (token == NEW_VAR) {
      uint8_t found = 0;
      for (uint8_t i = 0; i < _varc; i++){
        if (strcmp((char*)_vars[i].name, (char*)s->_id) == 0){
          found = i + 1; //TODO
          break;
        }
      }
      if (!found){
        _vars = (Variable*) realloc(_vars, (_varc + 1)*sizeof(Variable));
        _vars[_varc] = (Variable){.name = s->_id, .pos = s->_m - s->_m0};
        _varc++;
      } else {
        (_vars[found - 1]).pos = s->_m - s->_m0;
        free(s->_id);
      }
    } else if (token == STARTFUNCTION) {
      s->_lensrc = 0; // length src
      s->_fmatching = 1; // open/close
      _funcs = (RTFunction*) realloc(_funcs, (_funcc + 1)*sizeof(RTFunction));
      _funcs[_funcc] = (RTFunction){.name = s->_id, .src = NULL};
      _funcc++;
      s->_id = NULL;
      s->_idlen = 0;
      s->_srcinput = 1;
      return 0;
    } else {
      uint8_t found = 0;
      for (uint8_t i = 0; i < _varc; i++){
        if (strcmp((char*)_vars[i].name, (char*)s->_id) == 0){
          s->_m = s->_m0 + _vars[i].pos;
          found = 1;
          break;
        }
      }
      for (uint8_t i = 0; i < _funcc; i++){
        if (strcmp((char*)_funcs[i].name, (char*)s->_id) == 0){
          found = 1;
          new_sub((uint8_t*) _funcs[i].src, s);

          // TODO go back 1 token

          break;
        }
      }
      uint8_t fid = 0;
      while (!found){
        if (ext[fid].name == 0 || strcmp((char*)ext[fid].name, (char*)s->_id) == 0){
          int8_t ret_val = ext[fid].fp(s);
          if (ret_val) {
            return ret_val;
          }
          break;
        }
        fid++;
      }
      free(s->_id);
    }
    s->_id = NULL;
    s->_idlen = 0;
  }

  if (s->_lookahead){
    s->_lookahead = 0;
    switch (prev) {
      case POP:
        s->_stack_h--;
        switch (token) {
          case SUM:
          case SUB:
          case MULTI:
          case DIV:
          case MOD:
          case AND:
          case OR:
          case XOR:
            s->_prev_token = 0;
            step(LEFT, s);
            step(token, s);
            step(LOAD, s);
            break;
          default:
            s->_prev_token = 0;
            step(LEFT, s);
            step(LOAD, s);
            step(token, s);
        }
        break;
      case COND_MODIFIER:
        s->_cond = 1;
        switch (token) {
          case C_EQ:
            if (s->_type == INT8) s->_ans = REG.i8[0] == s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] == mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 == mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 == mload(s).f32;
            break;
          case C_NEQ:
            if (s->_type == INT8) s->_ans = REG.i8[0] != s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] != mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 != mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 != mload(s).f32;
            break;
          case C_LT:
            if (s->_type == INT8) s->_ans = REG.i8[0] < s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] < mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 < mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 < mload(s).f32;
            break;
          case C_GT:
            if (s->_type == INT8) s->_ans = REG.i8[0] > s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] > mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 > mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 > mload(s).f32;
            break;
          case C_LE:
            if (s->_type == INT8) s->_ans = REG.i8[0] <= s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] <= mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 <= mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 <= mload(s).f32;
            break;
          case C_GE:
            if (s->_type == INT8) s->_ans = REG.i8[0] >= s->_m[0];
            else if (s->_type == INT16) s->_ans = REG.i16[0] >= mload(s).i16[0];
            else if (s->_type == INT32) s->_ans = REG.i32 >= mload(s).i32;
            else if (s->_type == FLOAT) s->_ans = REG.f32 >= mload(s).f32;
            break;
          case C_NOT_NULL:
            if (s->_type == INT8) s->_ans = s->_m[0] != 0;
            else if (s->_type == INT16) s->_ans = mload(s).i16[0] != 0;
            else if (s->_type == INT32) s->_ans = mload(s).i32 != 0;
            else if (s->_type == FLOAT) s->_ans = mload(s).f32 != 0;
            break;
          case C_ZERO:
            if (s->_type == INT8) s->_ans = s->_m[0] == 0;
            else if (s->_type == INT16) s->_ans = mload(s).i16[0] == 0;
            else if (s->_type == INT32) s->_ans = mload(s).i32 == 0;
            else if (s->_type == FLOAT) s->_ans = mload(s).f32 == 0;
            break;
          case STACK_HEIGHT:
            s->_ans = s->_stack_h > 0;
            break;
          default:
            return JM_PEXC;
        }
        break;
      default:
        return JM_PEXC;
    }
    return 0;
  }

  switch (token) {
    case POP:
    case COND_MODIFIER:
      s->_lookahead = 1;
      break;
    case STRING:
      REG.i8[0] = 0;
      s->_string = 1;
      break;
    case RUN:
      {
        uint16_t len = strlen((char*) s->_m);
        uint8_t * src = (uint8_t*) malloc(len + 1);
        strcpy((char*) src, (char*) s->_m);
        new_sub(src, s);
        s->sub->_freesrc = 1;
      }
      break;
    case MALLOC:
      s->_mlen = REG.i16[0] * (s->_type + 1);
      uint16_t at = s->_m - s->_m0;
      s->_m0 = (uint8_t*) realloc(s->_m0, s->_mlen);
      s->_m = s->_m0 + at;
      memset(s->_m0 + 1, 0, s->_mlen - at - 1);
      break;
    case GTZERO:
      while (s->_m[0]) {
        s->_m++;
      }
      break;
    case RETURN:
      return -1;
      break;
    case LEFT:
      if (prev == BITWISE_OP){
        if (s->_type == INT8) s->_m[0] <<= REG.i8[0];
        else if (s->_type == INT16) {
          Register t = mload(s);
          t.i16[0] <<= REG.i16[0];
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
        } else if (s->_type == INT32) {
          Register t = mload(s);
          t.i32 <<= REG.i32;
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
          s->_m[2] = t.i8[2];
          s->_m[3] = t.i8[3];
        }
        return 0;
      } else {
        uint8_t mod = (prev >= '0' && prev <= '9') ? REG.i8[0] : 1;
        if (s->_type == INT16) mod *= 2;
        else if (s->_type != INT8) mod *= 4;
        if (s->_m - mod >= s->_m0) {
          s->_m -= mod;
        } else {
          return JM_REOB;
        }
        break;
      }
    case RIGHT:
      if (prev == BITWISE_OP){
        if (s->_type == INT8) s->_m[0] >>= REG.i8[0];
        else if (s->_type == INT16) {
          Register t = mload(s);
          t.i16[0] >>= REG.i16[0];
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
        } else if (s->_type == INT32) {
          Register t = mload(s);
          t.i32 >>= REG.i32;
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
          s->_m[2] = t.i8[2];
          s->_m[3] = t.i8[3];
        }
        return 0;
      } else {
        if (prev == STRING) {
          s->_m += REG.i8[0] + 1;
          break;
        }
        uint8_t mod = (prev >= '0' && prev <= '9') ? REG.i8[0] : 1;
        if (s->_type == INT16) mod *= 2;
        else if (s->_type != INT8) mod *= 4;
        if (s->_m + mod < s->_m0 + s->_mlen) {
          s->_m += mod;
        } else {
          return JM_REOB;
        }
        break;
      }
    case STACK_HEIGHT:
      REG.i16[0] = s->_stack_h;
      REG.i16[1] = 0;
      break;
    case PUSH:
      step(STORE, s);
      s->_prev_token = 0;
      step(RIGHT, s);
      s->_stack_h++;
      break;
    case ROTATE_REG:
      {
        uint8_t t = REG.i8[3];
        REG.i8[3] = REG.i8[2];
        REG.i8[2] = REG.i8[1];
        REG.i8[1] = REG.i8[0];
        REG.i8[0] = t;
      }
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

        if (!s->_cond && (token == WHILE || token == ENDWHILE)){
          if (s->_type == INT8) s->_ans = s->_m[0] != 0;
          else if (s->_type == INT16) s->_ans = mload(s).i16[0] != 0;
          else if (s->_type == INT32) s->_ans = mload(s).i32 != 0;
          else if (s->_type == FLOAT) s->_ans = mload(s).f32 != 0;
        }

        if (token == IF && !s->_ans){
          // jump to else : + 1 or endif ) + 1
          jmpmatch = JM_EIFE;
        } else if (token == ELSE){
          //jump to endif ) + 1
          jmpmatch = JM_ENIF;
        } else if (token == WHILE && !s->_ans){
          //jump to ENDWHILE ] + 1
          s->_cond = 0;
          jmpmatch = JM_EWHI;
        } else if (token == BREAK){
          //jump to ENDWHILE ] + 1
          jmpmatch = JM_NXWH;
        } else if (token == CONTINUE){
          //jump to while [
          s->_forward = 0;
          s->_matching = -1;
          jmpmatch = JM_WHI0;
        } else if (token == ENDWHILE && s->_ans){
          //jump to WHILE [ + 1
          s->_cond = 0;
          s->_forward = 0;
          s->_matching = -1;
          jmpmatch = JM_WHI1;
        }
        // printf("%c %d??\n", prev, jmpmatch);
        return jmpmatch;
      }
    case SUM:
      if (s->_type == INT8) s->_m[0] += REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] += REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 += REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      } else if (s->_type == FLOAT) {
        Register t = mload(s);
        t.f32 += REG.f32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case SUB:
      if (s->_type == INT8) s->_m[0] -= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] -= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 -= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      } else if (s->_type == FLOAT) {
        Register t = mload(s);
        t.f32 -= REG.f32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case MULTI:
      if (s->_type == INT8) s->_m[0] *= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] *= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 *= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      } else if (s->_type == FLOAT) {
        Register t = mload(s);
        t.f32 *= REG.f32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case DIV:
      if (s->_type == INT8) s->_m[0] /= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] /= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 /= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      } else if (s->_type == FLOAT) {
        Register t = mload(s);
        t.f32 /= REG.f32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case MOD:
      if (s->_type == INT8) s->_m[0] %= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] %= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 %= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case AND:
      if (s->_type == INT8) s->_m[0] &= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] &= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 &= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case OR:
      if (s->_type == INT8) s->_m[0] |= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        t.i16[0] |= REG.i16[0];
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
      } else if (s->_type == INT32) {
        Register t = mload(s);
        t.i32 |= REG.i32;
        s->_m[0] = t.i8[0];
        s->_m[1] = t.i8[1];
        s->_m[2] = t.i8[2];
        s->_m[3] = t.i8[3];
      }
      break;
    case XOR:
      if (prev == BITWISE_OP){
        if (s->_type == INT8) s->_m[0] ^= REG.i8[0];
        else if (s->_type == INT16) {
          Register t = mload(s);
          t.i16[0] ^= REG.i16[0];
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
        } else if (s->_type == INT32) {
          Register t = mload(s);
          t.i32 ^= REG.i32;
          s->_m[0] = t.i8[0];
          s->_m[1] = t.i8[1];
          s->_m[2] = t.i8[2];
          s->_m[3] = t.i8[3];
        }
      }
      break;
    case NOT:
      if (prev == BITWISE_OP){
        if (s->_type == INT8) REG.i8[0] = ~REG.i8[0];
        else if (s->_type == INT16) REG.i16[0] = ~REG.i16[0];
        else if (s->_type == INT32) REG.i32 = ~REG.i32;
      } else {
        s->_ans = !s->_ans;
        s->_cond = 1;
      }
      break;
    case SWITCH:
      if (s->_type == INT8)
        s->_m[0] ^= REG.i8[0],
        REG.i8[0] ^= s->_m[0],
        s->_m[0] ^= REG.i8[0];
      else if (s->_type == INT16) {
        Register t = mload(s);
        s->_m[0] = REG.i8[0];
        s->_m[1] = REG.i8[1];
        REG.i8[0] = t.i8[0];
        REG.i8[1] = t.i8[1];
      } else {
        Register t = mload(s);
        s->_m[0] = REG.i8[0];
        s->_m[1] = REG.i8[1];
        s->_m[2] = REG.i8[2];
        s->_m[3] = REG.i8[3];
        REG.i8[0] = t.i8[0];
        REG.i8[1] = t.i8[1];
        REG.i8[2] = t.i8[2];
        REG.i8[3] = t.i8[3];
      }
      break;
    case STORE:
      if (s->_type == INT8) s->_m[0] = REG.i8[0];
      else if (s->_type == INT16){
        s->_m[0] = REG.i8[0];
        s->_m[1] = REG.i8[1];
      } else {
        s->_m[0] = REG.i8[0];
        s->_m[1] = REG.i8[1];
        s->_m[2] = REG.i8[2];
        s->_m[3] = REG.i8[3];
      }
      break;
    case LOAD:
      if (s->_type == INT8) REG.i8[0] = s->_m[0];
      else if (s->_type == INT16) {
        REG.i8[0] = s->_m[0];
        REG.i8[1] = s->_m[1];
      } else {
        REG.i8[0] = s->_m[0];
        REG.i8[1] = s->_m[1];
        REG.i8[2] = s->_m[2];
        REG.i8[3] = s->_m[3];
      }
      break;
    case TYPE_CAST:
      if (s->_type == T_FLOAT) {
        REG.i32 = REG.f32;
      } else {
        REG.f32 = REG.i32;
      }
      break;
    case T_INT8:
      s->_type = 0;
      break;
    case T_INT16:
      s->_type = 1;
      break;
    case T_INT32:
      if (s->_type == FLOAT) {
        REG.i32 = (uint32_t) REG.f32;
      }
      s->_type = 2;
      break;
    case T_FLOAT:
      if (s->_type == INT32) {
        REG.f32 = (float) REG.i32;
      }
      s->_type = 3;
      break;
    case BOOL_TRUE:
      s->_ans = 1;
      s->_cond = 1;
      break;
    case ENDIF:
    case NOP:
    case DEBUG:
    case 0:
    case '\n':
    case '\t':
    case PRINT:
    case INPUT:
    case BITWISE_OP:
      break;
    default:
      if (token >= '0' && token <= '9'){
        if (prev < '0' || prev > '9'){
          if (s->_type == INT8) s->reg.i8[0] = (token - '0');
          else if (s->_type == INT16) s->reg.i16[0] = (token - '0');
          else if (s->_type == INT32) s->reg.i32 = (token - '0');
          else if (s->_type == FLOAT) s->reg.f32 = (token - '0');
        } else {
          if (s->_type == INT8) s->reg.i8[0] = s->reg.i8[0]*10 + (token - '0');
          else if (s->_type == INT16) s->reg.i16[0] = s->reg.i16[0]*10 + (token - '0');
          else if (s->_type == INT32) s->reg.i32 = s->reg.i32*10 + (token - '0');
          else if (s->_type == FLOAT) s->reg.f32 = s->reg.f32*10 + (token - '0');
        }
      } else {
        return JM_PEXC;
      }
      break;
  }
  return 0;
}
