#include <stdint.h>
#include <stdio.h>
#include <tokens.h>
#include <termios.h>
#include <unistd.h>
#include <debug_utils.h>

#define WIDTH 42

uint8_t getch() {
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

void print_state_mem(State * s) {
  printf("\033[K\033[90m[ ");
  uint8_t length = s->_mlen > WIDTH ? WIDTH : s->_mlen;
  for (uint8_t i = 0; i < length; i++) {
    if (s->_m0 + i == s->_m) {
      printf("\033[36m");
    }
    printf("%03d ", s->_m0[i]);
    if (s->_m0 + i == s->_m) {
      printf("\033[90m");
    }
  }
  printf("]\033[0m %ld %d\n", s->_m - s->_m0, s->_mlen);
}

void print_state_vars(State * s) {
  if (s->_varc == 0) return;
  printf("\033[K\033[90m  ");
  uint8_t length = s->_mlen > WIDTH ? WIDTH : s->_mlen;
  uint8_t type_len = 0;
  for (uint8_t i = 0; i < length; i++) {
    uint8_t found = 0;
    if (s->_m0 + i == s->_m) {
      printf("\033[4m\033[31m");
      type_len = s->_type == INT8 ? 1 : s->_type == INT16 ? 2 : s->_type == INT32 ? 4 : 4;
    } else if (type_len) {
      printf("\033[4m\033[31m");
      type_len--;
      if (!type_len) {
        printf("\033[24m\033[90m");
      }
    }
    for (uint8_t j = 0; j < s->_varc; j++) {
      if (s->_vars[j].pos == i) {
        printf("\033[31m");
        printf("%-3s", s->_vars[j].name);
        printf("\033[90m");
        found = 1;
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
  while (*src) {
    if (src == s->src) {
      printf("\033[42m");
    }
    if (*src == '\n') {
      printf("\\n");
    } else {
      printf("%c", *src);
    }
    if (src == s->src) {
      printf("\033[0m\033[90m");
    }
    src++;
  }
  if (src == s->src) {
    printf("\033[41m ");
  }
  printf("\033[0m\n");
}

uint8_t print_head(State * s) {
  char * rep = s->_type == INT8 ? "int8" : s->_type == INT16 ? "int16" : s->_type == INT32 ? "int32" : "float";
  char value[10];
  if (s->_type == INT8) sprintf(value, "%d", REG.i8[0]);
  else if (s->_type == INT16) sprintf(value, "%d", REG.i16[0]);
  else if (s->_type == INT32) sprintf(value, "%d", REG.i32);
  else if (s->_type == FLOAT) sprintf(value, "%f", REG.f32);
  printf("\033[K[%d %d %d %d:%d:%d(%s/%s)>  ", REG.i8[0], REG.i8[1], REG.i8[2], REG.i8[3], s->_ans, s->_type, rep, value);
  uint8_t c = getch();
  //usleep(100000);
  printf("(%c)", c);
  return c;
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
    if (s->src[0] == MARK) {
      printf("\n");
    } else {
      printf("\t\t- %c \n", s->src[0]);
    }
  }
  return 0;
}

uint8_t debug_state(State * s, uint8_t force_debug, uint8_t style){
  if (!force_debug && s->src[0] != MARK) return 0;

  if (force_debug == 0 && s->src[0] == MARK){
    print_step(s, 1);
  } else if (style == 0) {
    printf("\033[1F");
    print_step(s, 0);
    print_state_code(s);
  } else {
    uint8_t lines = 2;
    if (s->_varc > 0) lines = 3;
    printf("\033[%dF", lines);
    //print_mem(s->_m0, s->_mlen > 10 ? 10 : s->_mlen, s->_m - s->_m0);
    //print_code(s->_src0, s->src);
    print_state_vars(s);
    print_state_mem(s);
    print_state_code(s);
    uint8_t c = print_head(s);
    if (c == 'q') {
      return 1;
    }
  }
  return 0;
}
