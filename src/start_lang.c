#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <start_lang.h>

Variable * _vars = NULL;
uint8_t _varc = 0;
RTFunction * _funcs = NULL;
uint8_t _funcc = 0;

int8_t f_print(State * s);
int8_t f_input(State * s);

// Forward declarations
static uint8_t exec_math(uint8_t op, State *s, uint8_t prev);
static uint8_t exec_cmp(uint8_t op, State *s, uint8_t prev);
static uint8_t exec_var(uint8_t idx, State *s, uint8_t prev);
static uint8_t exec_func(uint8_t idx, State *s, uint8_t prev);
static uint8_t exec_misc(uint8_t op, State *s, uint8_t prev);
static uint8_t exec_const(uint8_t val, State *s);

static int is_id_start(char c) {
  return (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_id_cont(char c) {
  return is_id_start(c) || (c >= '0' && c <= '9');
}

static Register mload(State * s) {
  Register x = {.i32 = 0};
  if (s->_type == INT16) {
    x.i8[0] = s->_m[0];
    x.i8[1] = s->_m[1];
  } else if (s->_type != INT8) {
    x.i8[0] = s->_m[0];
    x.i8[1] = s->_m[1];
    x.i8[2] = s->_m[2];
    x.i8[3] = s->_m[3];
  } else {
    x.i8[0] = s->_m[0];
  }
  return x;
}

static void mstore(State * s, Register x) {
  if (s->_type == INT16) {
    s->_m[0] = x.i8[0];
    s->_m[1] = x.i8[1];
  } else if (s->_type != INT8) {
    s->_m[0] = x.i8[0];
    s->_m[1] = x.i8[1];
    s->_m[2] = x.i8[2];
    s->_m[3] = x.i8[3];
  } else {
    s->_m[0] = x.i8[0];
  }
}

void st_state_free(State * s) {
  if (s == NULL) return;
  if (s->_jumps) { free(s->_jumps); s->_jumps = NULL; }
  if (s->_freesrc && s->_src0) { free(s->_src0); s->_src0 = NULL; s->src = NULL; }
  
  for (int i = 0; i < MAX_FUNCS; i++) {
    if (s->func_src[i]) { free(s->func_src[i]); s->func_src[i] = NULL; }
    if (s->func_jumps[i]) { free(s->func_jumps[i]); s->func_jumps[i] = NULL; }
  }
  
  if (s->sub) {
    st_state_free(s->sub);
    s->sub = NULL;
  }
  
  if (s->_m0) {
    free(s->_m0);
    s->_m0 = NULL;
    s->_m = NULL;
  }
  free(s);
}

int8_t st_state_init(State * s) {
  if (!s->_m) {
    s->_m = (uint8_t*) malloc(sizeof(uint8_t) * MEM_SIZE);
    memset(s->_m, 0, sizeof(uint8_t) * MEM_SIZE);
    s->_m0 = s->_m;
    s->_mlen = MEM_SIZE;
    s->_src0 = s->src;
    s->_forward = 1;
    s->_op_result = JM_ERR0;
    s->reg.i8[0] = 1;
    s->_type = 0; // INT8
    s->_pending_char = 0;
    s->_pending_digit = 0;
    s->_has_pending_digit = 0;
  }
  return SUCCESS;
}

int8_t st_step(State * s) {
  if (s->sub) {
    int8_t r = st_step(s->sub);
    if (r != SUCCESS) {
      State * sub = s->sub;
      // Do NOT propagate sub cursor back to parent: functions have their
      // own logical cursor. Memory is shared (same _m0) so writes persist,
      // but the parent's _m stays put across a function call.
      // Clear shared pointers so st_state_free doesn't double-free.
      // sub borrows _m/_m0, _jumps (from parent's func_jumps[i]), and the
      // func_src/func_jumps arrays.
      sub->_m = NULL;
      sub->_m0 = NULL;
      sub->_jumps = NULL;
      memset(sub->func_src, 0, sizeof(sub->func_src));
      memset(sub->func_jumps, 0, sizeof(sub->func_jumps));
      st_state_free(sub);
      s->sub = NULL;
      if (r >= JM_ERR0 || r == BL_PREV) return r;
      return SUCCESS;
    }
    return SUCCESS;
  }

  uint8_t op = *(s->src);
  if (op == OP_END) return LOOP_ST;

  s->_op_result = st_op(op, s);
  
  if (s->sub) {
    st_state_init(s->sub);
    s->src++;
    return SUCCESS;
  }

  if (s->_op_result >= JM_ERR0) return s->_op_result;
  
  if (s->_op_result == SUCCESS) {
    s->src++;
  } else {
    // Control flow jump
    uint16_t pos = s->src - s->_src0;
    if (s->_jumps[pos] == 0) {
      if (s->_op_result == JM_WHI0 || s->_op_result == JM_WHI1) {
        return BL_PREV;
      }
      if (s->_op_result == JM_EWHI || s->_op_result == JM_NXWH ||
        s->_op_result == JM_EIFE || s->_op_result == JM_ENIF) {
        while (*(s->src)) s->src++;
        return SUCCESS;
      }
      return JM_PEXC;
    }
    s->src = s->_src0 + s->_jumps[pos];
    if (s->_op_result != JM_WHI0) {
      s->src++;
    }
  }
  
  return SUCCESS;
}

int8_t st_run(State * s) {
  if (s->_freesrc && s->_src0) {
    free(s->_src0);
    s->_src0 = NULL;
  }
  if (s->_jumps) {
    free(s->_jumps);
    s->_jumps = NULL;
  }

  st_state_init(s);

  uint16_t src_len = strlen((char *)s->src);
  uint16_t total_len = src_len + (s->_pending_char ? 1 : 0);
  // Increase multiplier to 8 to handle 1-char digits expanding to OP_CONST_I32/F32 (5 bytes) + extra
  uint8_t *src_copy = malloc(total_len * 8 + 1);
  int offset = 0;
  if (s->_pending_char) {
    src_copy[0] = s->_pending_char;
    s->_pending_char = 0;
    offset = 1;
  }
  strcpy((char*)src_copy + offset, (char*)s->src);
  s->src = src_copy;
  s->_src0 = src_copy;
  s->_freesrc = 1;

  if (s->_op_result == JM_WHI1 || s->_op_result == JM_WHI0) {
    uint8_t *raw = s->src;
    int matching = -1;
    int found_at = -1;
    for (int i = 0; raw[i]; i++) {
      if (raw[i] == '[') {
        if (++matching == 0) { found_at = i; break; }
      } else if (raw[i] == ']') {
        matching--;
      }
    }
    if (found_at < 0) {
      free(src_copy);
      s->_src0 = NULL;
      s->_freesrc = 0;
      s->_pending_char = 0;
      s->_has_pending_digit = 0;
      return BL_PREV;
    }
    {
      uint8_t *d = src_copy;
      const uint8_t *s = src_copy + found_at + 1;
      size_t n = strlen((char*)s) + 1;
      if (d < s) {
        while (n--) *d++ = *s++;
      } else if (d > s) {
        d += n;
        s += n;
        while (n--) *--d = *--s;
      }
    }
    s->_op_result = SUCCESS;
    s->_pending_char = 0;
    s->_has_pending_digit = 0;
  }

  uint16_t *jumps = calloc(total_len * 8 + 1, sizeof(uint16_t));
  int8_t prep_res = st_prepare(s->src, jumps, total_len * 8 + 1, s);
  if (prep_res != SUCCESS) {
    free(jumps);
    return prep_res;
  }
  s->_jumps = jumps;

  if (s->_op_result == JM_WHI1 || s->_op_result == JM_WHI0) {
    uint8_t *p = s->src;
    int matching = -1;
    while (1) {
      uint8_t b = *p;
      if (b == OP_CONST_I8) { p += 2; continue; }
      if (b == OP_CONST_I16) { p += 3; continue; }
      if (b == OP_CONST_I32 || b == OP_CONST_F32) { p += 5; continue; }
      if (b == OP_STRING) { p += 2 + (uint8_t)p[1]; continue; }
      if (b == OP_END) break;
      if (b == OP_WHILE) {
        if (++matching == 0) {
          s->src = p + 1;
          s->_op_result = SUCCESS;
          break;
        }
      } else if (b == OP_ENDWHILE) {
        matching--;
      }
      p++;
    }
    if (s->_op_result == JM_WHI1 || s->_op_result == JM_WHI0) {
      free(jumps);
      s->_jumps = NULL;
      s->_pending_char = 0;
      s->_has_pending_digit = 0;
      return BL_PREV;
    }
  }

  while (1) {
    State * sub = s;
    while (sub->sub) sub = sub->sub;
    if (step_callback(sub) != 0) return LOOP_ST;
    int8_t r = st_step(s);
    if (r == LOOP_ST) break;
    if (r == BL_PREV) {
      s->_pending_char = 0;
      s->_has_pending_digit = 0;
      return BL_PREV;
    }
    if (r >= JM_ERR0) return r;
  }
  return BL_NEXT;
}

uint8_t st_op(uint8_t op, State *s) {
  uint8_t prev = s->_prev_token;
  s->_prev_token = op;
  if (op >= OP_CONST_MIN && op <= OP_CONST_MAX) return exec_const(op - OP_CONST_MIN, s);
  if (op >= OP_MATH_FIRST && op <= OP_MATH_LAST) return exec_math(op, s, prev);
  if (op >= OP_CMP_FIRST && op <= OP_CMP_LAST) return exec_cmp(op, s, prev);
  if (op >= OP_TYPE_FIRST && op <= OP_TYPE_LAST) {
    uint8_t new_type = op - OP_TYPE_FIRST;
    // src_old semantics: converting between FLOAT and INT32 via type switch
    // reinterprets/casts the register value; narrow integer switches leave
    // the register bytes untouched.
    if (new_type == INT32 && s->_type == FLOAT) {
      s->reg.i32 = (uint32_t) s->reg.f32;
    } else if (new_type == FLOAT && s->_type == INT32) {
      s->reg.f32 = (float) s->reg.i32;
    }
    s->_type = new_type;
    return SUCCESS;
  }
  if (op >= OP_VAR_FIRST && op <= OP_VAR_LAST) return exec_var(op - OP_VAR_FIRST, s, prev);
  if (op >= OP_FUNC_FIRST && op <= OP_FUNC_LAST) return exec_func(op - OP_FUNC_FIRST, s, prev);
  return exec_misc(op, s, prev);
}

static uint8_t exec_const(uint8_t val, State *s) {
  if (s->_type == 3) s->reg.f32 = (float)val;
  else if (s->_type == 2) s->reg.i32 = val;
  else if (s->_type == 1) s->reg.i16[0] = val;
  else s->reg.i8[0] = val;
  return SUCCESS;
}

static uint8_t exec_math(uint8_t op, State *s, uint8_t prev) {
  Register m = mload(s);
  int type = s->_type;
  switch (op) {
    case OP_ADD:
      if (type == 3) m.f32 += s->reg.f32;
      else if (type == 2) m.i32 += s->reg.i32;
      else if (type == 1) m.i16[0] += s->reg.i16[0];
      else m.i8[0] += s->reg.i8[0];
      break;
    case OP_SUB:
      if (type == 3) m.f32 -= s->reg.f32;
      else if (type == 2) m.i32 -= s->reg.i32;
      else if (type == 1) m.i16[0] -= s->reg.i16[0];
      else m.i8[0] -= s->reg.i8[0];
      break;
    case OP_MUL:
      if (type == 3) m.f32 *= s->reg.f32;
      else if (type == 2) m.i32 *= s->reg.i32;
      else if (type == 1) m.i16[0] *= s->reg.i16[0];
      else m.i8[0] *= s->reg.i8[0];
      break;
    case OP_DIV:
      if (type == 3) { if (s->reg.f32 != 0) m.f32 /= s->reg.f32; }
      else if (type == 2) { if (s->reg.i32 != 0) m.i32 /= s->reg.i32; }
      else if (type == 1) { if (s->reg.i16[0] != 0) m.i16[0] /= s->reg.i16[0]; }
      else { if (s->reg.i8[0] != 0) m.i8[0] /= s->reg.i8[0]; }
      break;
    case OP_MOD:
      if (type == 2) { if (s->reg.i32 != 0) m.i32 %= s->reg.i32; }
      else if (type == 1) { if (s->reg.i16[0] != 0) m.i16[0] %= s->reg.i16[0]; }
      else if (type == 0) { if (s->reg.i8[0] != 0) m.i8[0] %= s->reg.i8[0]; }
      break;
    case OP_AND:
      if (type == 2) m.i32 &= s->reg.i32;
      else if (type == 1) m.i16[0] &= s->reg.i16[0];
      else m.i8[0] &= s->reg.i8[0];
      break;
    case OP_OR:
      if (type == 2) m.i32 |= s->reg.i32;
      else if (type == 1) m.i16[0] |= s->reg.i16[0];
      else m.i8[0] |= s->reg.i8[0];
      break;
    case OP_XOR:
      if (type == 2) m.i32 ^= s->reg.i32;
      else if (type == 1) m.i16[0] ^= s->reg.i16[0];
      else m.i8[0] ^= s->reg.i8[0];
      break;
    case OP_SHL:
      if (type == 2) m.i32 <<= s->reg.i32;
      else if (type == 1) m.i16[0] <<= s->reg.i16[0];
      else m.i8[0] <<= s->reg.i8[0];
      break;
    case OP_SHR:
      if (type == 2) m.i32 >>= s->reg.i32;
      else if (type == 1) m.i16[0] >>= s->reg.i16[0];
      else m.i8[0] >>= s->reg.i8[0];
      break;
    case OP_NOT_BIT:
      if (type == 3) return SUCCESS; // No bitwise on float
      if (type == 2) s->reg.i32 = ~s->reg.i32;
      else if (type == 1) s->reg.i16[0] = ~s->reg.i16[0];
      else s->reg.i8[0] = ~s->reg.i8[0];
      return SUCCESS;
  }
  mstore(s, m);
  return SUCCESS;
}


static uint8_t exec_cmp(uint8_t op, State *s, uint8_t prev) {
  s->_cond = 1;
  Register m = mload(s);
  int type = s->_type;
  switch (op) {
    case OP_CMP_EQ:
      if (type == 3) s->_ans = (m.f32 == s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 == s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] == s->reg.i16[0]);
      else s->_ans = (m.i8[0] == s->reg.i8[0]);
      break;
    case OP_CMP_NEQ:
      if (type == 3) s->_ans = (m.f32 != s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 != s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] != s->reg.i16[0]);
      else s->_ans = (m.i8[0] != s->reg.i8[0]);
      break;
    case OP_CMP_LT:
      if (type == 3) s->_ans = (m.f32 < s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 < s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] < s->reg.i16[0]);
      else s->_ans = (m.i8[0] < s->reg.i8[0]);
      break;
    case OP_CMP_GT:
      if (type == 3) s->_ans = (m.f32 > s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 > s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] > s->reg.i16[0]);
      else s->_ans = (m.i8[0] > s->reg.i8[0]);
      break;
    case OP_CMP_LE:
      if (type == 3) s->_ans = (m.f32 <= s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 <= s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] <= s->reg.i16[0]);
      else s->_ans = (m.i8[0] <= s->reg.i8[0]);
      break;
    case OP_CMP_GE:
      if (type == 3) s->_ans = (m.f32 >= s->reg.f32);
      else if (type == 2) s->_ans = (m.i32 >= s->reg.i32);
      else if (type == 1) s->_ans = (m.i16[0] >= s->reg.i16[0]);
      else s->_ans = (m.i8[0] >= s->reg.i8[0]);
      break;
    case OP_CMP_NZ:
      if (type == 3) s->_ans = (m.f32 != 0);
      else if (type == 2) s->_ans = (m.i32 != 0);
      else if (type == 1) s->_ans = (m.i16[0] != 0);
      else s->_ans = (m.i8[0] != 0);
      break;
    case OP_CMP_ZERO:
      if (type == 3) s->_ans = (m.f32 == 0);
      else if (type == 2) s->_ans = (m.i32 == 0);
      else if (type == 1) s->_ans = (m.i16[0] == 0);
      else s->_ans = (m.i8[0] == 0);
      break;
    case OP_CMP_STACK:
      s->_ans = (s->_stack_h > 0);
      break;
    case OP_CMP_TRUE:
      s->_ans = 1;
      break;
    case OP_CMP_INV:
      s->_ans = !s->_ans;
      break;
  }
  return SUCCESS;
}

static uint8_t exec_var(uint8_t idx, State *s, uint8_t prev) {
  if (s->src[1] == OP_NEW_VAR) {
    s->var_pos[idx] = s->_m - s->_m0;
    // Compatibility: update globals
    if (_vars && idx < _varc) {
      _vars[idx].pos = s->var_pos[idx];
    }
    s->src++;
  } else {
    s->_m = s->_m0 + s->var_pos[idx];
  }
  return SUCCESS;
}

static void new_sub(uint8_t * src, uint16_t * jumps, State * s) {
  State* sub = (State*) malloc(sizeof(State));
  memset(sub, 0, sizeof(State));
  sub->src = src;
  sub->_m = s->_m;
  sub->_m0 = s->_m0;
  sub->_mlen = s->_mlen;
  sub->_src0 = sub->src;
  sub->_jumps = jumps;
  sub->varc = s->varc;
  sub->funcc = s->funcc;
  memcpy(sub->var_pos, s->var_pos, sizeof(s->var_pos));
  memcpy(sub->func_src, s->func_src, sizeof(s->func_src));
  memcpy(sub->func_jumps, s->func_jumps, sizeof(s->func_jumps));
  memcpy(sub->func_names, s->func_names, sizeof(s->func_names));
  memcpy(sub->ext_fp, s->ext_fp, sizeof(s->ext_fp));
  s->sub = sub;
}

static uint8_t exec_func(uint8_t idx, State *s, uint8_t prev) {
  if (idx < MAX_FUNCS) {
    if (s->func_src[idx]) {
      new_sub(s->func_src[idx], s->func_jumps[idx], s);
      return SUCCESS;
    } else if (s->ext_fp[idx]) {
      // For compatibility with undef and other tools that use s->_id
      strcpy((char*)s->_id, (char*)s->func_names[idx]);
      return s->ext_fp[idx](s);
    }
  }
  return SUCCESS;
}

static uint8_t exec_misc(uint8_t op, State *s, uint8_t prev) {
  switch (op) {
    case OP_IF:
      if (!s->_ans) return JM_EIFE;
      break;
    case OP_ELSE:
      return JM_ENIF;
    case OP_ENDIF:
      break;
    case OP_DEBUG:
      break;
    case OP_WHILE:
      if (!s->_cond) {
        Register m = mload(s);
        if (s->_type == 3) s->_ans = (m.f32 != 0);
        else if (s->_type == 2) s->_ans = (m.i32 != 0);
        else if (s->_type == 1) s->_ans = (m.i16[0] != 0);
        else s->_ans = (m.i8[0] != 0);
      }
      s->_cond = 0;
      if (!s->_ans) return JM_EWHI;
      break;
    case OP_ENDWHILE:
      if (!s->_cond) {
        Register m = mload(s);
        if (s->_type == 3) s->_ans = (m.f32 != 0);
        else if (s->_type == 2) s->_ans = (m.i32 != 0);
        else if (s->_type == 1) s->_ans = (m.i16[0] != 0);
        else s->_ans = (m.i8[0] != 0);
      }
      s->_cond = 0;
      if (s->_ans) return JM_WHI1;
      break;
    case OP_BREAK:
      return JM_NXWH;
    case OP_CONTINUE:
      return JM_WHI0;
    case OP_RETURN:
      return -1;
    case OP_RUN: {
      uint16_t len = strlen((char*) s->_m);
      uint8_t * src = (uint8_t*) malloc(len + 1);
      strcpy((char*) src, (char*) s->_m);
      uint16_t *jumps = calloc(len * 8 + 1, sizeof(uint16_t));
      st_prepare(src, jumps, len * 8 + 1, s);
      new_sub(src, jumps, s);
      s->sub->_freesrc = 1;
      break;
    }
    case OP_LEFT: {
      int mod_count = ((prev >= OP_CONST_MIN && prev <= OP_CONST_MAX) || 
               prev == OP_CONST_I8 || prev == OP_CONST_I16 || 
               prev == OP_CONST_I32 || prev == OP_CONST_F32) ? s->reg.i8[0] : 1;
      int type_size = (s->_type == 1) ? 2 : (s->_type > 1 ? 4 : 1);
      int mod = mod_count * type_size;
      if (s->_m - mod >= s->_m0) s->_m -= mod;
      else return JM_REOB;
      break;
    }
    case OP_RIGHT: {
      int mod_count = 1;
      if (prev == OP_STRING) {
        mod_count = s->reg.i8[0] + 1;
      } else if ((prev >= OP_CONST_MIN && prev <= OP_CONST_MAX) || 
             prev == OP_CONST_I8 || prev == OP_CONST_I16 || 
             prev == OP_CONST_I32 || prev == OP_CONST_F32) {
        mod_count = s->reg.i8[0];
      }
      int type_size = (s->_type == 1) ? 2 : (s->_type > 1 ? 4 : 1);
      int mod = (prev == OP_STRING) ? mod_count : mod_count * type_size;
      if (s->_m + mod < s->_m0 + s->_mlen) s->_m += mod;
      else return JM_REOB;
      break;
    }
    case OP_STORE:
      mstore(s, s->reg);
      break;
    case OP_LOAD:
      // Preserve unused upper bytes of reg for narrower types
      // (matches src_old behaviour, relied upon by ROTATE tests).
      if (s->_type == INT8) {
        s->reg.i8[0] = s->_m[0];
      } else if (s->_type == INT16) {
        s->reg.i8[0] = s->_m[0];
        s->reg.i8[1] = s->_m[1];
      } else {
        s->reg = mload(s);
      }
      break;
    case OP_SWITCH: {
      Register m = mload(s);
      mstore(s, s->reg);
      s->reg = m;
      break;
    }
    case OP_GTZERO:
      while (*(s->_m)) s->_m++;
      break;
    case OP_MALLOC: {
      uint16_t size = s->reg.i16[0] * (s->_type + 1);
      uint16_t at = s->_m - s->_m0;
      s->_m0 = (uint8_t*) realloc(s->_m0, size);
      s->_m = s->_m0 + at;
      s->_mlen = size;
      break;
    }
    case OP_ROTATE: {
      uint8_t t = s->reg.i8[3];
      s->reg.i8[3] = s->reg.i8[2];
      s->reg.i8[2] = s->reg.i8[1];
      s->reg.i8[1] = s->reg.i8[0];
      s->reg.i8[0] = t;
      break;
    }
    case OP_CAST:
      if (s->_type == 3) s->reg.i32 = (uint32_t)s->reg.f32;
      else s->reg.f32 = (float)s->reg.i32;
      break;
    case OP_PUSH:
      mstore(s, s->reg);
      s->_m += (s->_type == 1 ? 2 : (s->_type > 1 ? 4 : 1));
      s->_stack_h++;
      break;
    case OP_POP: {
      s->_m -= (s->_type == 1 ? 2 : (s->_type > 1 ? 4 : 1));
      s->_stack_h--;
      uint8_t next_op = s->src[1];
      if (next_op >= OP_MATH_FIRST && next_op <= OP_MATH_LAST) {
        s->src++;
        s->_prev_token = 0; // Reset prev token so math op doesn't see 'o'
        uint8_t res = st_op(next_op, s);
        s->reg = mload(s);
        return res;
      } else {
        s->reg = mload(s);
      }
      break;
    }
    case OP_STACK_H:
      s->reg.i16[0] = s->_stack_h;
      break;
    case OP_PRINT:
      f_print(s);
      break;
    case OP_INPUT:
      f_input(s);
      break;
    case OP_STRING: {
      uint8_t len = s->src[1];
      s->src += 2;
      // Need room for `len` bytes + 1 null terminator.
      if ((s->_m - s->_m0) + len >= s->_mlen) return JM_REOB;
      memcpy(s->_m, s->src, len);
      s->_m[len] = 0;
      // Note: REG.i8[0] should store length for some reason?
      // In original code REG.i8[0] was used as counter during string capture.
      s->reg.i8[0] = len;
      s->src += len - 1; // st_step will increment again
      break;
    }
    case OP_CONST_I8:
      exec_const(s->src[1], s);
      s->src++;
      break;
    case OP_CONST_I16: {
      uint16_t val;
      memcpy(&val, s->src + 1, 2);
      if (s->_type == 3) s->reg.f32 = (float)val;
      else if (s->_type == 2) s->reg.i32 = (uint32_t)val;
      else if (s->_type == 1) s->reg.i16[0] = val;
      else s->reg.i8[0] = (uint8_t)val;
      s->src += 2;
      break;
    }
    case OP_CONST_I32: {
      uint32_t val;
      memcpy(&val, s->src + 1, 4);
      if (s->_type == 3) s->reg.f32 = (float)val;
      else if (s->_type == 2) s->reg.i32 = val;
      else if (s->_type == 1) s->reg.i16[0] = (uint16_t)val;
      else s->reg.i8[0] = (uint8_t)val;
      s->src += 4;
      break;
    }
    case OP_CONST_F32: {
      float val;
      memcpy(&val, s->src + 1, 4);
      s->reg.f32 = val;
      s->src += 4;
      break;
    }
    default:
      return JM_PEXC;
  }
  return SUCCESS;
}

typedef struct {
  char name[MAX_IDLEN];
  uint16_t pos;
  uint8_t *func_src;
  uint16_t *func_jumps;
  int8_t (*ext_fp)(struct _State * s);
} SymEntry;

static int8_t st_prepare_internal(uint8_t *buf, uint16_t *jumps, State *s, SymEntry *vars, uint8_t *varc, SymEntry *funcs, uint8_t *funcc);

int8_t st_prepare(uint8_t *buf, uint16_t *jumps, uint16_t buf_len, State *s) {
  SymEntry vars[MAX_VARS];
  SymEntry funcs[MAX_FUNCS];
  uint8_t vc = 0, fc = 0;
  memset(vars, 0, sizeof(vars));
  memset(funcs, 0, sizeof(funcs));

  int8_t res = st_prepare_internal(buf, jumps, s, vars, &vc, funcs, &fc);
  if (res != SUCCESS) return res;

  s->varc = vc;
  s->funcc = fc;
  for (int i = 0; i < vc; i++) {
    s->var_pos[i] = vars[i].pos;
    // Sincronizar com global _vars para ferramentas de debug
    if (i >= _varc) {
      _vars = realloc(_vars, (i + 1) * sizeof(Variable));
      {
        size_t len = strlen((char*)vars[i].name) + 1;
        _vars[i].name = (uint8_t*)malloc(len);
        if (_vars[i].name) memcpy(_vars[i].name, vars[i].name, len);
      }
      _varc = i + 1;
    }
    _vars[i].pos = vars[i].pos;
  }
  for (int i = 0; i < fc; i++) {
    s->func_src[i] = funcs[i].func_src;
    s->func_jumps[i] = funcs[i].func_jumps;
    s->ext_fp[i] = funcs[i].ext_fp;
    strcpy((char*)s->func_names[i], funcs[i].name);
  }
  return SUCCESS;
}

static int8_t st_prepare_internal(uint8_t *buf, uint16_t *jumps, State *s, SymEntry *vars, uint8_t *varc, SymEntry *funcs, uint8_t *funcc) {
  uint8_t *r = buf;
  uint8_t *w = buf;
  uint16_t stack[ST_MAX_DEPTH];
  uint8_t type_stack[ST_MAX_DEPTH]; // 0 for IF, 1 for WHILE
  uint16_t break_stack[ST_MAX_DEPTH][MAX_BREAKS_PER_LOOP];
  uint8_t break_count[ST_MAX_DEPTH];
  int depth = 0;
  int current_type = s->_type;
  memset(break_count, 0, sizeof(break_count));

  // Criar uma cópia temporária do buffer para leitura
  uint16_t blen = strlen((char*)buf);
  uint8_t *read_buf = malloc(blen + 1);
  memcpy(read_buf, buf, blen + 1);
  r = read_buf;

  while (*r) {
    if (s->_has_pending_digit && (*r < '0' || *r > '9')) {
      uint32_t val = s->_pending_digit;
      s->_has_pending_digit = 0;
      if (current_type == 3) {
        float f = (float)val;
        *w++ = OP_CONST_F32;
        memcpy(w, &f, 4); w += 4;
      } else if (current_type == 2) {
        *w++ = OP_CONST_I32; memcpy(w, &val, 4); w += 4;
      } else if (current_type == 1) {
        *w++ = OP_CONST_I16; uint16_t v16 = (uint16_t)val; memcpy(w, &v16, 2); w += 2;
      } else {
        if (val <= 63) *w++ = OP_CONST_MIN + (uint8_t)val;
        else { *w++ = OP_CONST_I8; *w++ = (uint8_t)val; }
      }
    }

    if (*r == ' ' || *r == '\n' || *r == '\r' || *r == '\t') { r++; continue; }
    
    uint8_t op = OP_END;
    int advance = 1;

    if (*r == '(') op = OP_IF;
    else if (*r == ':') op = OP_ELSE;
    else if (*r == ')') op = OP_ENDIF;
    else if (*r == '[') op = OP_WHILE;
    else if (*r == ']') op = OP_ENDWHILE;
    else if (*r == 'x') op = OP_BREAK;
    else if (*r == 'c') op = OP_CONTINUE;
    else if (*r == 'r') op = OP_RETURN;
    else if (*r == '#') op = OP_RUN;
    else if (*r == '`') op = OP_DEBUG;
    else if (*r == '!') op = OP_STORE;
    else if (*r == ';') op = OP_LOAD;
    else if (*r == '@') op = OP_SWITCH;
    else if (*r == 'z') op = OP_GTZERO;
    else if (*r == 'm') op = OP_MALLOC;
    else if (*r == 'k') op = OP_ROTATE;
    else if (*r == 'e') op = OP_CAST;
    else if (*r == '+') op = OP_ADD;
    else if (*r == '-') op = OP_SUB;
    else if (*r == '*') op = OP_MUL;
    else if (*r == '%') op = OP_MOD;
    else if (*r == '&') op = OP_AND;
    else if (*r == '|') op = OP_OR;
    else if (*r == 'p') op = OP_PUSH;
    else if (*r == 'o') {
      if (r[1] == '\0') { 
        if (s->_ignend) { s->_pending_char = *r; *w = OP_END; free(read_buf); return SUCCESS; }
        free(read_buf); return JM_EXEN; 
      }
      *w++ = OP_POP;
      r++;
      while (*r && (*r == ' ' || *r == '\n' || *r == '\r' || *r == '\t')) r++;
      if (*r == '+' || *r == '-' || *r == '*' || *r == '/' || *r == '%') {
        continue; 
      }
      *w++ = OP_LOAD;
      continue;
    }
    else if (*r == 'h') op = OP_STACK_H;
    else if (*r == '.') op = OP_PRINT;
    else if (*r == ',') op = OP_INPUT;
    else if (*r == '<') op = OP_LEFT;
    else if (*r == '>') op = OP_RIGHT;
    else if (*r == '~') op = OP_CMP_INV;
    else if (*r == 't') op = OP_CMP_TRUE;
    else if (*r == 'b') { op = OP_TYPE_I8; current_type = 0; }
    else if (*r == 's') { op = OP_TYPE_I16; current_type = 1; }
    else if (*r == 'i') { op = OP_TYPE_I32; current_type = 2; }
    else if (*r == 'f') { op = OP_TYPE_F32; current_type = 3; }
    else if (*r == '/') {
      if (r[1] == '\0') { 
        if (s->_ignend) { s->_pending_char = *r; *w = OP_END; free(read_buf); return SUCCESS; }
        free(read_buf); return JM_EXEN; 
      }
      if (r[1] == '/') { while (*r && *r != '\n') r++; continue; }
      if (r[1] == '*') { r += 2; while (*r && !(*r == '*' && r[1] == '/')) r++; if (*r) r += 2; continue; }
      op = OP_DIV;
    }
    else if (*r == 'w') {
      if (r[1] == '\0') { 
        if (s->_ignend) { s->_pending_char = *r; *w = OP_END; free(read_buf); return SUCCESS; }
        free(read_buf); return JM_EXEN; 
      }
      r++; advance = 1;
      if (*r == '&') op = OP_AND;
      else if (*r == '|') op = OP_OR;
      else if (*r == '^') op = OP_XOR;
      else if (*r == '~') op = OP_NOT_BIT;
      else if (*r == '<') op = OP_SHL;
      else if (*r == '>') op = OP_SHR;
      else { r--; op = OP_END; }
    }
    else if (*r == '?') {
      if (r[1] == '\0') { 
        if (s->_ignend) { s->_pending_char = *r; *w = OP_END; free(read_buf); return SUCCESS; }
        free(read_buf); return JM_EXEN; 
      }
      r++; advance = 1;
      if (*r == '=') op = OP_CMP_EQ;
      else if (*r == '!') op = OP_CMP_NEQ;
      else if (*r == '<') op = OP_CMP_LT;
      else if (*r == '>') op = OP_CMP_GT;
      else if (*r == 'l') op = OP_CMP_LE;
      else if (*r == 'g') op = OP_CMP_GE;
      else if (*r == '?') op = OP_CMP_NZ;
      else if (*r == 'z') op = OP_CMP_ZERO;
      else if (*r == 'h') op = OP_CMP_STACK;
      else { r--; op = OP_CMP_NZ; advance = 1; }
    }
    else if (*r == '"') {
      *w++ = OP_STRING;
      uint8_t *len_ptr = w++;
      r++;
      uint8_t len = 0;
      // Legacy escape semantics: a backslash marks the next byte as
      // literal. Chains of backslashes stay in "escape" state while
      // still writing a backslash, so `\\"` writes `\"` without
      // terminating the string, matching src_old behaviour.
      int esc = 0;
      while (*r) {
        if (esc) {
          if (*r == 'n') { *w++ = '\n'; esc = 0; }
          else if (*r == 't') { *w++ = '\t'; esc = 0; }
          else if (*r == '\\') { *w++ = '\\'; /* esc stays */ }
          else { *w++ = *r; esc = 0; }
          r++; len++;
        } else if (*r == '\\') {
          esc = 1;
          r++;
        } else if (*r == '"') {
          break;
        } else {
          *w++ = *r;
          r++; len++;
        }
      }
      if (*r) r++;
      *len_ptr = len;
      continue;
    }
    else if (*r >= '0' && *r <= '9') {
      uint32_t val = 0;
      if (s->_has_pending_digit) {
        val = s->_pending_digit;
      }
      while (*r >= '0' && *r <= '9') {
        val = val * 10 + (*r - '0');
        r++;
      }
      if (*r == '\0' && s->_ignend) {
        s->_pending_digit = val;
        s->_has_pending_digit = 1;
        *w = OP_END;
        free(read_buf);
        return SUCCESS;
      }
      s->_has_pending_digit = 0;
      if (current_type == 3) {
        // For floats, we might need a more complex strategy if split at '.'
        // but let's handle the simple case first.
        float f = (float)val;
        if (*r == '.') {
          r++;
          float frac = 0.1f;
          while (*r >= '0' && *r <= '9') {
            f += (*r - '0') * frac;
            frac /= 10.0f;
            r++;
          }
        }
        *w++ = OP_CONST_F32;
        memcpy(w, &f, 4); w += 4;
      } else if (current_type == 2) {
        *w++ = OP_CONST_I32; memcpy(w, &val, 4); w += 4;
      } else if (current_type == 1) {
        *w++ = OP_CONST_I16; uint16_t v16 = (uint16_t)val; memcpy(w, &v16, 2); w += 2;
      } else {
        if (val <= 63) *w++ = OP_CONST_MIN + (uint8_t)val;
        else { *w++ = OP_CONST_I8; *w++ = (uint8_t)val; }
      }
      continue;
    }
    else if (is_id_start(*r)) {
      char name[MAX_IDLEN]; int len = 0;
      while (*r && is_id_cont(*r)) { if (len < MAX_IDLEN - 1) name[len++] = *r; r++; }
      name[len] = 0;
      if (*r == '^') {
        int idx = -1;
        for (int i = 0; i < *varc; i++) if (!strcmp(vars[i].name, name)) idx = i;
        if (idx == -1 && *varc < MAX_VARS) { idx = (*varc)++; strcpy(vars[idx].name, name); }
        if (idx != -1) { *w++ = OP_VAR_FIRST + idx; *w++ = OP_NEW_VAR; }
        r++;
      } else if (*r == '{') {
        int idx = -1;
        for (int i = 0; i < *funcc; i++) if (!strcmp(funcs[i].name, name)) idx = i;
        if (idx == -1 && *funcc < MAX_FUNCS) { idx = (*funcc)++; strcpy(funcs[idx].name, name); }
        r++; 
        uint8_t *start = r;
        int fdepth = 1;
        while (*r && fdepth > 0) {
          if (*r == '{') fdepth++;
          else if (*r == '}') fdepth--;
          r++;
        }
        if (idx != -1) {
          int flen = r - start - 1;
          // Allocate with 8x multiplier to account for opcode
          // expansion (e.g. digit sequences expand to OP_CONST_I32
          // + 4 bytes, strings get OP_STRING+len prefix, etc.).
          int fcap = flen * 8 + 1;
          funcs[idx].func_src = malloc(fcap);
          memcpy(funcs[idx].func_src, start, flen);
          funcs[idx].func_src[flen] = 0;
          funcs[idx].func_jumps = calloc(fcap, sizeof(uint16_t));
          st_prepare_internal(funcs[idx].func_src, funcs[idx].func_jumps, s, vars, varc, funcs, funcc);
        }
        continue;
      } else {
        int idx = -1;
        for (int i = 0; i < *varc; i++) if (!strcmp(vars[i].name, name)) idx = i;
        if (idx != -1) *w++ = OP_VAR_FIRST + idx;
        else {
          idx = -1;
          for (int i = 0; i < *funcc; i++) if (!strcmp(funcs[i].name, name)) idx = i;
          if (idx == -1) {
            for (int i = 0; ext[i].name; i++) {
              if (!strcmp((char*)ext[i].name, name)) {
                if (*funcc < MAX_FUNCS) {
                  idx = (*funcc)++;
                  strcpy(funcs[idx].name, name);
                  funcs[idx].func_src = NULL;
                  funcs[idx].ext_fp = ext[i].fp;
                }
                break;
              }
            }
          }
          if (idx != -1) *w++ = OP_FUNC_FIRST + idx;
          else {
            // try to find the "undef" handler (NULL name in ext)
            int i = 0; while (ext[i].name != NULL) i++;
            if (*funcc < MAX_FUNCS) {
              idx = (*funcc)++;
              strcpy(funcs[idx].name, name);
              funcs[idx].func_src = NULL;
              funcs[idx].ext_fp = ext[i].fp;
              *w++ = OP_FUNC_FIRST + idx;
            }
          }
        }
      }
      continue;
    }

    if (op != OP_END) {
      uint16_t pos = w - buf;
      *w++ = op;
      if (op == OP_WHILE || op == OP_IF) {
        if (depth < ST_MAX_DEPTH) {
          stack[depth] = pos;
          type_stack[depth] = (op == OP_WHILE) ? 1 : 0;
          break_count[depth] = 0;
          depth++;
        }
      } else if (op == OP_CONTINUE) {
        // jump to nearest WHILE start
        for (int i = depth - 1; i >= 0; i--) {
          if (type_stack[i] == 1) {
            jumps[pos] = stack[i];
            break;
          }
        }
      } else if (op == OP_BREAK) {
        // store position to patch when WHILE ends
        for (int i = depth - 1; i >= 0; i--) {
          if (type_stack[i] == 1) {
            if (break_count[i] < MAX_BREAKS_PER_LOOP) {
              break_stack[i][break_count[i]++] = pos;
            }
            break;
          }
        }
      } else if (op == OP_ENDWHILE || op == OP_ENDIF || op == OP_ELSE) {
        if (depth > 0) {
          uint16_t open = stack[--depth];
          jumps[open] = pos;
          jumps[pos] = open;
          if (op == OP_ENDWHILE) {
            // patch all 'x' inside this loop
            for (int i = 0; i < break_count[depth]; i++) {
              jumps[break_stack[depth][i]] = pos;
            }
          }
          if (op == OP_ELSE) {
            stack[depth] = pos;
            type_stack[depth] = 0;
            break_count[depth] = 0;
            depth++;
          }
        }
      }
    } else {
      free(read_buf);
      return JM_PEXC;
    }
    r += advance;
  }
  *w = OP_END;
  free(read_buf);
  return SUCCESS;
}
