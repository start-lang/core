#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <start_tokens.h>
#include <time.h>
#include <debug_utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

#define WIDTH 130

typedef struct {
  uint8_t* name;
  uint16_t pos;
  uint8_t type;
} TypedVariable;

TypedVariable * vars = NULL;
uint8_t varc = 0;

clock_t start_time;
uint8_t breakpoint = 0;
uint8_t first_callback = 1;
uint8_t follow_vars = 0;
uint8_t follow_mem = 0;
uint16_t srclen = 0;
uint32_t timeout = 0;
uint16_t mem_offset = 0;
uint16_t code_offset = 0;
uint16_t mem_used = 0;
uint32_t max_steps = 0;
uint16_t max_output = 0;
uint64_t steps = 0;
uint32_t forward_steps = 0;
uint32_t time_spent = 0;
uint16_t forward_multiply = 1;
extern uint16_t output_len;

extern Variable * _vars;
extern uint8_t _varc;

#if defined(__linux__) || defined(__APPLE__)
uint8_t getch(void) {
  struct termios oldt, newt;
  uint8_t ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  if (ch == '\033') {
    getchar();
    switch (getchar()) {
      case 'A': ch = 1; break; // up
      case 'B': ch = 2; break; // down
      case 'C': ch = 3; break; // right
      case 'D': ch = 4; break; // left
      default: break;
    }
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}
#endif

uint8_t print_num(uint8_t t, uint8_t *m) {
  Register r = {.i32 = m[0] | (m[1] << 8) | (m[2] << 16) | (m[3] << 24)};
  if (t == INT8) return printf("%d", r.i8[0]);
  else if (t == INT16) return printf("%d", r.i16[0]);
  else if (t == INT32) return printf("%d", r.i32);
  else return printf("%f", r.f32);
}

void print_state_mem(State * s) {
  printf("\033[K\033[90m");
  if (mem_offset) {
    printf("\033[0m%3d\033[90m", mem_offset);
  } else {
    printf("  0");
  }
  printf("[ ");
  uint8_t max_len = (WIDTH - 15)/4;
  uint8_t length = s->_mlen - mem_offset > max_len ? max_len : s->_mlen - mem_offset;
  uint8_t show_value = 1;
  for (uint8_t i = mem_offset; i < length + mem_offset; i++) {
    TypedVariable * v = NULL;
    for (uint8_t j = 0; j < varc; j++) {
      if (vars[j].pos == i) {
        v = vars + j;
        break;
      }
    }
    if (show_value) {
      if (s->_m0 + i == s->_m) {
        if (v && s->_type != v->type) {
          v = NULL;
          printf("\033[33m");
        } else {
          printf("\033[36m");
        }
      }
      if (!v) {
        printf("%03d ", s->_m0[i]);
      } else {
        uint8_t vlen = 0;
        uint8_t type = v->type;
        uint8_t type_len = type == INT8 ? 1 : type == INT16 ? 2 : type == INT32 ? 4 : 4;
        if (i + type_len - 1 > length + mem_offset) {
          printf("%03d ", s->_m0[i]);
          continue;
        }

        vlen += print_num(type, s->_m0 + i);
        for (uint8_t j = 0; j < type_len * 4 - vlen; j++) {
          printf(" ");
        }
        i += type_len - 1;
        printf("\033[90m");
        continue;
      }
      if (s->_m0 + i == s->_m) {
        printf("\033[90m");
      }
    } else {
      if (s->_m0 + i == s->_m) {
        if (v && s->_type != v->type) {
          v = NULL;
          printf("\033[33m");
        } else {
          printf("\033[36m");
        }
      }
      printf("%03d ", s->_m0[i]);
      if (s->_m0 + i == s->_m) {
        printf("\033[90m");
      }
    }
  }
  printf("][%d/%d]\n", length, s->_mlen);
}

void print_state_vars(State * s) {
  printf("\033[K\033[90m     ");
  uint16_t current_pos = s->_m - s->_m0;
  uint8_t type_len = s->_type == INT8 ? 1 : s->_type == INT16 ? 2 : s->_type == INT32 ? 4 : 4;
  uint8_t max_len = (WIDTH - 15)/4;
  uint8_t length = s->_mlen - mem_offset > max_len ? max_len : s->_mlen - mem_offset;
  if (current_pos + type_len - 1 < mem_offset) {
    mem_offset = current_pos;
  } else if (current_pos + type_len - 1 >= mem_offset + max_len) {
    mem_offset = current_pos - max_len + type_len;
  }
  uint8_t type_hl = 0;
  for (uint8_t i = mem_offset; i < length + mem_offset; i++) {
    uint8_t found = 0;
    if (s->_m0 + i == s->_m) {
      printf("\033[4m\033[31m");
      type_hl = type_len;
    } else if (type_hl) {
      printf("\033[4m\033[31m");
      type_hl--;
      if (!type_hl) {
        printf("\033[24m\033[90m");
      }
    }
    for (uint8_t j = 0; j < varc; j++) {
      if (vars[j].pos == i) {
        printf("\033[31m");
        printf("%-3s", vars[j].name);
        printf("\033[90m");
        found = 1;
        break;
      }
    }
    if (!found) {
      printf("   ");
    }
    printf("\033[0m \033[90m");
  }
  printf("\033[0m\n");
}

void print_state_code(State * s) {
  printf("\033[K\033[90m");
  uint8_t * src = s->_src0;
  uint16_t i = 0;
  while (*src) {
    if (src == s->src) {
      printf("\033[42m");
    }
    if (*src == '\n') {
      printf("\\n");
      i++;
    } else if (*src == DEBUG) {
      printf("\033[33m%c\033[90m", *src);
    } else {
      printf("%c", *src);
    }
    if (src == s->src) {
      printf("\033[0m\033[90m");
    }
    src++;
    i++;
    if (i > WIDTH) {
      i = 0;
      printf("\n");
    }
  }
  if (src == s->src) {
    printf("\033[41m ");
  }
  printf("\033[0m\n");
}

void print_head(State * s) {
  char t = s->_type == INT8 ? 'b' : s->_type == INT16 ? 's' : s->_type == INT32 ? 'i' : 'f';
  char i = s->_cond ? s->_ans ? 't' : 'f' : '?';
  printf("\033[K[%d %d %d %d:%c:%c(", REG.i8[0], REG.i8[1], REG.i8[2], REG.i8[3], i, t);

  if (s->_type == INT8) printf("%d", REG.i8[0]);
  else if (s->_type == INT16) printf("%d", REG.i16[0]);
  else if (s->_type == INT32) printf("%d", REG.i32);
  else if (s->_type == FLOAT) printf("%f", REG.f32);
  printf(")>  f %d F %d x%d", 10 * forward_multiply, 100 * forward_multiply, forward_multiply);
}

uint8_t print_step(State * s, uint8_t color) {
  if (color) {

  } else {
    for (uint8_t * i = s->_m0; i <= s->_m + 2; i++){
      if (i == s->_m) {
        printf("[%d:%d:%d> ", REG.i32, s->_ans, s->_type);
      }
      printf("[%d] ", i[0]);
    }
    if (s->src[0] == DEBUG) {
      printf("\n");
    } else {
      printf("\t\t- %c \n", s->src[0]);
    }
  }
  return 0;
}

void flush_term(uint8_t lines) {
  printf("\033[u\033[K");
  for (uint8_t i = 0; i < lines; i++) {
    printf("\n\033[K");
  }
  printf("\033[u\033[K");
}

uint8_t debug_state(State * s, uint8_t enable, uint8_t interactive){
  if (first_callback) {
    start_time = clock();
    first_callback = 0;
    uint8_t * src = s->_src0;
    while (*src) {
      if (*src == DEBUG) {
        follow_vars = 1;
      }
      src++;
      srclen++;
    }
  } else if (steps > 10000 && s->src - s->_src0 == srclen) {
    time_spent = (clock() - start_time) * 1000 / CLOCKS_PER_SEC;
  }

  uint8_t stop = 0;
  if (max_steps && steps >= max_steps - 1) stop = 1;
  if (max_output && output_len >= max_output) stop = 1;
  steps++;

  if (timeout && steps % 10000 == 0) {
    if ((clock() - start_time) * 1000 / CLOCKS_PER_SEC > timeout) stop = 1;
  }

  if (s->src[0] == DEBUG) breakpoint = enable = interactive = 1;

  if (breakpoint) enable = interactive = 1;

  if (follow_vars && s->_prev_token == NEW_VAR) {
    vars = (TypedVariable*) realloc(vars, (varc + 1) * sizeof(TypedVariable));
    Variable * v = _vars + _varc - 1;
    vars[varc] = (TypedVariable){.name = v->name, .pos = v->pos, .type = s->_type};
    varc++;
  }

  if (follow_mem) {
    uint8_t type_len = s->_type == INT8 ? 1 : s->_type == INT16 ? 2 : s->_type == INT32 ? 4 : 4;
    uint16_t current_pos = s->_m - s->_m0;
    if (current_pos + type_len > mem_used) {
      mem_used = current_pos + type_len;
    }
  }

  if (!enable && s->src[0] != DEBUG) return stop;

  if (enable == 0 && s->src[0] == DEBUG && 0){
    print_step(s, 1);
  } else if (interactive == 0) {
    printf("\033[1F");
    print_step(s, 0);
    print_state_code(s);
  } else {
    printf("\033[s");

    uint8_t lines = 4 + strlen((char*) s->_src0) / WIDTH;

    if (forward_steps) {
      forward_steps--;
      return 0;
    }
    printf("\n");
    print_state_vars(s);
    print_state_mem(s);
    print_state_code(s);
    print_head(s);

    int8_t c = getch();
    switch (c) {
      case 1: // up
        break;
      case 2: // down
        break;
      case 3: // right
        break;
      case 4: // left
        break;
      case 'q':
        stop = 1;
        flush_term(lines);
        break;
      case 'f':
        forward_steps = 10 * forward_multiply;
        break;
      case 'F':
        forward_steps = 100 * forward_multiply;
        break;
      case 'x':
        if (forward_multiply < 1000) forward_multiply *= 10;
        break;
      case 'z':
        if (forward_multiply > 10) forward_multiply /= 10;
        break;
      case 'c':
        breakpoint = enable = interactive = 0;
        flush_term(lines);
        return 0;
      case 'C':
        forward_steps = 1000000000;
        flush_term(lines);
        return 0;
      default:
        break;
    }
    if (s->src - s->_src0 == srclen) {
      flush_term(lines);
    } else {
      printf("\033[%dF\033[u\033[K", lines);
    }
  }
  return stop;
}

void print_exec_info(void) {
  if (!steps) return;
  printf("\n\n%"PRIu64" op\n", steps);
  if (follow_mem) printf("%d bytes\n", mem_used);
  printf("%.2f s\n", time_spent/1000.0);
}