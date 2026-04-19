#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <start_lang.h>


int8_t f_print(State * s);
int8_t f_input(State * s);

// Forward declarations
static void call_push(State *s, uint8_t *src, uint16_t *jumps);
static void call_pop(State *s);
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

static Register mget(State *s) {
  Register x = {.i32 = 0};
  x.i8[0] = s->_m[0];
  if (s->_type >= INT16) x.i8[1] = s->_m[1];
  if (s->_type >= INT32) { x.i8[2] = s->_m[2]; x.i8[3] = s->_m[3]; }
  return x;
}

static void mset(State *s, Register x) {
  s->_m[0] = x.i8[0];
  if (s->_type >= INT16) s->_m[1] = x.i8[1];
  if (s->_type >= INT32) { s->_m[2] = x.i8[2]; s->_m[3] = x.i8[3]; }
}

static float mgetf(State *s) {
  float f;
  memcpy(&f, s->_m, 4);
  return f;
}

static void msetf(State *s, float f) {
  memcpy(s->_m, &f, 4);
}

void st_state_free(State *s) {
  if (s == NULL) return;
  if (s->_jumps) { free(s->_jumps); s->_jumps = NULL; }
  if (s->_freesrc && s->_src0) { free(s->_src0); s->_src0 = NULL; s->src = NULL; }
  if (s->_m0) { free(s->_m0); s->_m0 = NULL; s->_m = NULL; }
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
    s->_last_result = JM_ERR0;
    s->reg.i8[0] = 1;
    s->_type = 0;
    s->_pending_char = 0;
    s->_pending_digit = 0;
    s->_has_pending_digit = 0;
  }
  return SUCCESS;
}

int8_t st_step(State *s) {
  uint8_t op = *(s->src);
  if (op == OP_END) {
    if (s->_depth > 0) {
      call_pop(s);
      // src is already pointing to the instruction after the call opcode
      return SUCCESS;
    }
    return LOOP_ST;
  }

  uint8_t depth_before = s->_depth;
  uint8_t r = st_op(op, s);

  if (s->_depth > depth_before) {
    // call_push was invoked; advance parent's saved src past the call opcode
    s->_frames[s->_depth - 1].src++;
    return SUCCESS;
  }

  // OP_RETURN returns uint8_t(-1) = 255; handle it as "end of call frame"
  if (r == 255) {
    if (s->_depth > 0) {
      call_pop(s);
      s->src++;
      return SUCCESS;
    }
    return LOOP_ST;
  }

  s->_last_result = r;
  if (r >= JM_ERR0) return r;

  if (r == SUCCESS) {
    s->src++;
  } else {
    // control flow jump
    uint16_t pos = s->src - s->_src0;
    if (s->_jumps[pos] == 0) {
      if (r == JM_WHI0 || r == JM_WHI1) return BL_PREV;
      if (r == JM_EWHI || r == JM_NXWH || r == JM_EIFE || r == JM_ENIF) {
        while (*(s->src)) s->src++;
        return SUCCESS;
      }
      return JM_PEXC;
    }
    s->src = s->_src0 + s->_jumps[pos];
    if (r != JM_WHI0) s->src++;
  }

  return SUCCESS;
}

// Clean (non-streaming) runner. Compiles src and executes it in one shot.
// Does not use or preserve _pending_char, _pending_digit, or _last_result.
int8_t st_run(State * s) {
  if (s->_freesrc && s->_src0) { free(s->_src0); s->_src0 = NULL; }
  if (s->_jumps) { free(s->_jumps); s->_jumps = NULL; }

  st_state_init(s);

  uint16_t src_len = strlen((char *)s->src);
  // multiplier 8: a single source byte can expand to up to 5 compiled bytes
  uint8_t *src_copy = malloc(src_len * 8 + 1);
  strcpy((char*)src_copy, (char*)s->src);
  s->src = src_copy;
  s->_src0 = src_copy;
  s->_freesrc = 1;

  uint16_t *jumps = calloc(src_len * 8 + 1, sizeof(uint16_t));
  int8_t prep_res = st_prepare(s->src, jumps, src_len * 8 + 1, s);
  if (prep_res != SUCCESS) { free(jumps); return prep_res; }
  s->_jumps = jumps;

  while (1) {
    if (step_callback(s) != 0) return LOOP_ST;
    int8_t r = st_step(s);
    if (r == LOOP_ST) break;
    if (r >= JM_ERR0) return r;
  }
  return BL_NEXT;
}

// Streaming runner. Accepts source in small chunks across multiple calls.
// Preserves _pending_char, _pending_digit, and _last_result between calls.
// Returns BL_PREV (-1) when the caller should rewind one chunk and retry,
// BL_NEXT (0) when the chunk completed normally, or an error code.
int8_t st_run_stream(State * s) {
  if (s->_freesrc && s->_src0) { free(s->_src0); s->_src0 = NULL; }
  if (s->_jumps) { free(s->_jumps); s->_jumps = NULL; }

  st_state_init(s);

  uint16_t src_len = strlen((char *)s->src);
  uint16_t total_len = src_len + (s->_pending_char ? 1 : 0);
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

  // if last chunk left us mid-loop, scan raw text for the matching '[' start
  if (s->_last_result == JM_WHI1 || s->_last_result == JM_WHI0) {
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
    // shift the buffer so it starts right after the '['
    uint8_t *d = src_copy;
    const uint8_t *p = src_copy + found_at + 1;
    size_t n = strlen((char*)p) + 1;
    if (d < p) {
      while (n--) *d++ = *p++;
    } else if (d > p) {
      d += n; p += n;
      while (n--) *--d = *--p;
    }
    s->_last_result = SUCCESS;
    s->_pending_char = 0;
    s->_has_pending_digit = 0;
  }

  uint16_t *jumps = calloc(total_len * 8 + 1, sizeof(uint16_t));
  int8_t prep_res = st_prepare(s->src, jumps, total_len * 8 + 1, s);
  if (prep_res != SUCCESS) { free(jumps); return prep_res; }
  s->_jumps = jumps;

  // after compilation, locate the '[' entry point if resuming a loop
  if (s->_last_result == JM_WHI1 || s->_last_result == JM_WHI0) {
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
        if (++matching == 0) { s->src = p + 1; s->_last_result = SUCCESS; break; }
      } else if (b == OP_ENDWHILE) {
        matching--;
      }
      p++;
    }
    if (s->_last_result == JM_WHI1 || s->_last_result == JM_WHI0) {
      free(jumps);
      s->_jumps = NULL;
      s->_pending_char = 0;
      s->_has_pending_digit = 0;
      return BL_PREV;
    }
  }

  while (1) {
    if (step_callback(s) != 0) return LOOP_ST;
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
  if (s->_type == FLOAT) {
    float m = mgetf(s);
    switch (op) {
      case OP_ADD: msetf(s, m + s->reg.f32); break;
      case OP_SUB: msetf(s, m - s->reg.f32); break;
      case OP_MUL: msetf(s, m * s->reg.f32); break;
      case OP_DIV: if (s->reg.f32 != 0) msetf(s, m / s->reg.f32); break;
      default: break; // no bitwise/mod on float
    }
    return SUCCESS;
  }
  Register m = mget(s);
  int type = s->_type;
  switch (op) {
    case OP_ADD:
      if (type == INT32) m.i32 += s->reg.i32;
      else if (type == INT16) m.i16[0] += s->reg.i16[0];
      else m.i8[0] += s->reg.i8[0];
      break;
    case OP_SUB:
      if (type == INT32) m.i32 -= s->reg.i32;
      else if (type == INT16) m.i16[0] -= s->reg.i16[0];
      else m.i8[0] -= s->reg.i8[0];
      break;
    case OP_MUL:
      if (type == INT32) m.i32 *= s->reg.i32;
      else if (type == INT16) m.i16[0] *= s->reg.i16[0];
      else m.i8[0] *= s->reg.i8[0];
      break;
    case OP_DIV:
      if (type == INT32) { if (s->reg.i32 != 0) m.i32 /= s->reg.i32; }
      else if (type == INT16) { if (s->reg.i16[0] != 0) m.i16[0] /= s->reg.i16[0]; }
      else { if (s->reg.i8[0] != 0) m.i8[0] /= s->reg.i8[0]; }
      break;
    case OP_MOD:
      if (type == INT32) { if (s->reg.i32 != 0) m.i32 %= s->reg.i32; }
      else if (type == INT16) { if (s->reg.i16[0] != 0) m.i16[0] %= s->reg.i16[0]; }
      else { if (s->reg.i8[0] != 0) m.i8[0] %= s->reg.i8[0]; }
      break;
    case OP_AND:
      if (type == INT32) m.i32 &= s->reg.i32;
      else if (type == INT16) m.i16[0] &= s->reg.i16[0];
      else m.i8[0] &= s->reg.i8[0];
      break;
    case OP_OR:
      if (type == INT32) m.i32 |= s->reg.i32;
      else if (type == INT16) m.i16[0] |= s->reg.i16[0];
      else m.i8[0] |= s->reg.i8[0];
      break;
    case OP_XOR:
      if (type == INT32) m.i32 ^= s->reg.i32;
      else if (type == INT16) m.i16[0] ^= s->reg.i16[0];
      else m.i8[0] ^= s->reg.i8[0];
      break;
    case OP_SHL:
      if (type == INT32) m.i32 <<= s->reg.i32;
      else if (type == INT16) m.i16[0] <<= s->reg.i16[0];
      else m.i8[0] <<= s->reg.i8[0];
      break;
    case OP_SHR:
      if (type == INT32) m.i32 >>= s->reg.i32;
      else if (type == INT16) m.i16[0] >>= s->reg.i16[0];
      else m.i8[0] >>= s->reg.i8[0];
      break;
    case OP_NOT_BIT:
      if (type == INT32) s->reg.i32 = ~s->reg.i32;
      else if (type == INT16) s->reg.i16[0] = ~s->reg.i16[0];
      else s->reg.i8[0] = ~s->reg.i8[0];
      return SUCCESS;
  }
  mset(s, m);
  return SUCCESS;
}


static uint8_t exec_cmp(uint8_t op, State *s, uint8_t prev) {
  s->_cond = 1;
  int type = s->_type;
  Register m = (type == FLOAT) ? (Register){.f32 = mgetf(s)} : mget(s);
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
    s->src++;
  } else {
    s->_m = s->_m0 + s->var_pos[idx];
  }
  return SUCCESS;
}

static void call_push(State *s, uint8_t *src, uint16_t *jumps) {
  if (s->_depth >= ST_MAX_CALL_DEPTH) return;
  CallFrame *f = &s->_frames[s->_depth++];
  f->src = s->src;
  f->src0 = s->_src0;
  f->jumps = s->_jumps;
  f->_m = s->_m;
  f->_m0 = s->_m0;
  f->_mlen = s->_mlen;
  f->_type = s->_type;
  f->_ans = s->_ans;
  f->_cond = s->_cond;
  f->_prev_token = s->_prev_token;
  f->_matching = s->_matching;
  f->_freesrc = 0; // new frame's freesrc starts clean; set by caller if needed
  // function sees tape from current cursor; _m0 shifts to current position
  s->_mlen = s->_mlen - (s->_m - s->_m0);
  s->_m0 = s->_m;
  s->src = src;
  s->_src0 = src;
  s->_jumps = jumps;
  s->_type = 0;
  s->_ans = 0;
  s->_cond = 0;
  s->_prev_token = 0;
  s->_matching = 0;
  s->_freesrc = 0;
}

static void call_pop(State *s) {
  if (s->_depth == 0) return;
  // free heap-allocated src/jumps only for OP_RUN path
  if (s->_freesrc) {
    if (s->_src0) { free(s->_src0); }
    if (s->_jumps) { free(s->_jumps); }
  }
  s->_freesrc = 0;
  s->_jumps = NULL;
  CallFrame *f = &s->_frames[--s->_depth];
  s->src = f->src;
  s->_src0 = f->src0;
  s->_jumps = f->jumps;
  s->_m = f->_m;
  s->_m0 = f->_m0;
  s->_mlen = f->_mlen;
  s->_type = f->_type;
  s->_ans = f->_ans;
  s->_cond = f->_cond;
  s->_prev_token = f->_prev_token;
  s->_matching = f->_matching;
}

static uint8_t exec_func(uint8_t idx, State *s, uint8_t prev) {
  if (idx >= MAX_FUNCS) return SUCCESS;
  if (s->ext_fp[idx]) {
    strcpy((char*)s->_id, (char*)s->func_names[idx]);
    return s->ext_fp[idx](s);
  }
  if (idx < s->funcc) {
    call_push(s, s->func_body[idx], s->func_jmp[idx]);
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
        if (s->_type == FLOAT) s->_ans = (mgetf(s) != 0);
        else { Register m = mget(s); s->_ans = (s->_type == INT32) ? (m.i32 != 0) : (s->_type == INT16) ? (m.i16[0] != 0) : (m.i8[0] != 0); }
      }
      s->_cond = 0;
      if (!s->_ans) return JM_EWHI;
      break;
    case OP_ENDWHILE:
      if (!s->_cond) {
        if (s->_type == FLOAT) s->_ans = (mgetf(s) != 0);
        else { Register m = mget(s); s->_ans = (s->_type == INT32) ? (m.i32 != 0) : (s->_type == INT16) ? (m.i16[0] != 0) : (m.i8[0] != 0); }
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
      uint8_t *src = (uint8_t*) malloc(len * 8 + 1);
      strcpy((char*) src, (char*) s->_m);
      uint16_t *jumps = calloc(len * 8 + 1, sizeof(uint16_t));
      st_prepare(src, jumps, len * 8 + 1, s);
      call_push(s, src, jumps);
      // mark the pushed frame's src as heap-allocated so it's freed on pop
      s->_freesrc = 1;
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
      if (s->_type == FLOAT) msetf(s, s->reg.f32);
      else mset(s, s->reg);
      break;
    case OP_LOAD:
      // preserve upper bytes for narrower types (ROTATE tests rely on this)
      if (s->_type == INT8) {
        s->reg.i8[0] = s->_m[0];
      } else if (s->_type == INT16) {
        s->reg.i8[0] = s->_m[0];
        s->reg.i8[1] = s->_m[1];
      } else if (s->_type == FLOAT) {
        s->reg.f32 = mgetf(s);
      } else {
        s->reg = mget(s);
      }
      break;
    case OP_SWITCH: {
      if (s->_type == FLOAT) {
        float m = mgetf(s);
        msetf(s, s->reg.f32);
        s->reg.f32 = m;
      } else {
        Register m = mget(s);
        mset(s, s->reg);
        s->reg = m;
      }
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
      if (size > s->_mlen) memset(s->_m0 + s->_mlen, 0, size - s->_mlen);
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
      if (s->_type == FLOAT) msetf(s, s->reg.f32);
      else mset(s, s->reg);
      s->_m += (s->_type == INT16 ? 2 : (s->_type > INT16 ? 4 : 1));
      s->_stack_h++;
      break;
    case OP_POP: {
      s->_m -= (s->_type == INT16 ? 2 : (s->_type > INT16 ? 4 : 1));
      s->_stack_h--;
      uint8_t next_op = s->src[1];
      if (next_op >= OP_MATH_FIRST && next_op <= OP_MATH_LAST) {
        s->src++;
        s->_prev_token = 0;
        uint8_t res = st_op(next_op, s);
        if (s->_type == FLOAT) s->reg.f32 = mgetf(s);
        else s->reg = mget(s);
        return res;
      } else {
        if (s->_type == FLOAT) s->reg.f32 = mgetf(s);
        else s->reg = mget(s);
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
  int8_t (*ext_fp)(struct _State * s);
} SymEntry;

static int8_t st_prepare_internal(uint8_t *buf, uint16_t *jumps, State *s, SymEntry *vars, uint8_t *varc, SymEntry *funcs, uint8_t *funcc);

int8_t st_prepare(uint8_t *buf, uint16_t *jumps, uint16_t buf_len, State *s) {
  SymEntry vars[MAX_VARS];
  SymEntry funcs[MAX_FUNCS];
  memset(vars, 0, sizeof(vars));
  memset(funcs, 0, sizeof(funcs));

  // seed local tables from existing state so nested calls (e.g. OP_RUN)
  // don't erase outer functions and variables
  uint8_t vc = s->varc, fc = s->funcc;
  for (uint8_t i = 0; i < vc; i++) {
    strncpy(vars[i].name, (char*)s->var_names[i], MAX_IDLEN - 1);
    vars[i].name[MAX_IDLEN - 1] = 0;
    vars[i].pos = s->var_pos[i];
  }
  for (uint8_t i = 0; i < fc; i++) {
    strncpy(funcs[i].name, (char*)s->func_names[i], MAX_IDLEN - 1);
    funcs[i].name[MAX_IDLEN - 1] = 0;
    funcs[i].ext_fp = s->ext_fp[i];
  }

  int8_t res = st_prepare_internal(buf, jumps, s, vars, &vc, funcs, &fc);
  if (res != SUCCESS) return res;

  s->varc = vc;
  s->funcc = fc;
  for (int i = 0; i < vc; i++) {
    s->var_pos[i] = vars[i].pos;
    strncpy((char*)s->var_names[i], vars[i].name, MAX_IDLEN - 1);
    s->var_names[i][MAX_IDLEN - 1] = 0;
  }
  for (int i = 0; i < fc; i++) {
    s->ext_fp[i] = funcs[i].ext_fp;
    strcpy((char*)s->func_names[i], funcs[i].name);
  }
  return SUCCESS;
}

// Resolve an identifier reference (not a definition) to a bytecode opcode.
// Searches vars, then funcs, then ext[], then falls back to the undef handler.
// Writes one opcode byte to *w and advances *w. Returns 1 on success, 0 if
// *w was not advanced (e.g. funcc exhausted).
static int resolve_id(const char *name, SymEntry *vars, uint8_t *varc,
                      SymEntry *funcs, uint8_t *funcc, uint8_t **w) {
  // variable reference
  for (int i = 0; i < *varc; i++) {
    if (!strcmp(vars[i].name, name)) {
      **w = OP_VAR_FIRST + i;
      (*w)++;
      return 1;
    }
  }
  // known function (previously registered)
  for (int i = 0; i < *funcc; i++) {
    if (!strcmp(funcs[i].name, name)) {
      **w = OP_FUNC_FIRST + i;
      (*w)++;
      return 1;
    }
  }
  // external function from ext[] table
  for (int i = 0; ext[i].name; i++) {
    if (!strcmp((char*)ext[i].name, name)) {
      if (*funcc < MAX_FUNCS) {
        int idx = (*funcc)++;
        strcpy(funcs[idx].name, name);
        funcs[idx].ext_fp = ext[i].fp;
        **w = OP_FUNC_FIRST + idx;
        (*w)++;
        return 1;
      }
      return 0;
    }
  }
  // fallback: undef handler (ext entry with NULL name)
  int i = 0;
  while (ext[i].name != NULL) i++;
  if (*funcc < MAX_FUNCS) {
    int idx = (*funcc)++;
    strcpy(funcs[idx].name, name);
    funcs[idx].ext_fp = ext[i].fp;
    **w = OP_FUNC_FIRST + idx;
    (*w)++;
    return 1;
  }
  return 0;
}

static int8_t st_prepare_internal(uint8_t *buf, uint16_t *jumps, State *s, SymEntry *vars, uint8_t *varc, SymEntry *funcs, uint8_t *funcc) {
  uint8_t *r = buf;
  uint8_t *w = buf;
  uint16_t stack[ST_MAX_CTRL_DEPTH];
  uint8_t type_stack[ST_MAX_CTRL_DEPTH]; // 0 for IF, 1 for WHILE
  uint16_t break_stack[ST_MAX_CTRL_DEPTH][MAX_BREAKS_PER_LOOP];
  uint8_t break_count[ST_MAX_CTRL_DEPTH];
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
          if (flen >= MAX_FUNC_BODY) { free(read_buf); return JM_REOB; }
          // copy source text into static pool; st_prepare_internal reads via
          // an internal copy and writes bytecode back into the same buffer
          memcpy(s->func_body[idx], start, flen);
          s->func_body[idx][flen] = 0;
          memset(s->func_jmp[idx], 0, sizeof(s->func_jmp[idx]));
          // clear ext_fp so the compiled body takes precedence over any
          // previously registered undef/ext handler for this slot
          funcs[idx].ext_fp = NULL;
          st_prepare_internal(s->func_body[idx], s->func_jmp[idx], s, vars, varc, funcs, funcc);
        }
        continue;
      } else {
        resolve_id(name, vars, varc, funcs, funcc, &w);
      }
      continue;
    }

    if (op != OP_END) {
      uint16_t pos = w - buf;
      *w++ = op;
      if (op == OP_WHILE || op == OP_IF) {
        if (depth < ST_MAX_CTRL_DEPTH) {
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
