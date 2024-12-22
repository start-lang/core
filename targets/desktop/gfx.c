#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <simplegfx.h>
#include <inttypes.h>
#include <start_lang.h>
#include <microcuts.h>

uint8_t stop = 0;
uint16_t output_len = 0;

int8_t f_print(State * s);
int8_t f_printstr(State * s);
int8_t f_printnum(State * s);
int8_t f_debug_num(State * s);
int8_t exception_handler(State * s);
int8_t undef(State * s);
int8_t f_error(State * s);
int8_t f_quit(State * s);

#ifndef STDF_IGN_PRINT
int8_t f_print(State * s){
  output_len += gfx_printf("%c", (char) s->_m[0]);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTSTR
int8_t f_printstr(State * s){
  // string:str(_m) ... '/0'
  output_len += gfx_printf("%s", (char*) s->_m);
  return 0;
}
#endif

#ifndef STDF_IGN_PRINTNUM
int8_t f_printnum(State * s){
  // num:8/16/32/f(r0) -> _m:str
  char * m = (char *) s->_m;
  if (s->_type == INT8) s->reg.i8[0] = sprintf(m, "%d", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = sprintf(m, "%d", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = sprintf(m, "%d", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = sprintf(m, "%f", s->reg.f32);
  return 0;
}
#endif

#ifndef STDF_IGN_DEBUG_NUM
int8_t f_debug_num(State * s){
  // num:8/16/32/f(r0)
  if (s->_type == INT8) output_len += s->reg.i8[0] = gfx_printf("%d\r\n", s->reg.i8[0]);
  else if (s->_type == INT16) output_len += s->reg.i16[0] = gfx_printf("%d\r\n", s->reg.i16[0]);
  else if (s->_type == INT32) output_len += s->reg.i32 = gfx_printf("%d\r\n", s->reg.i32);
  else if (s->_type == FLOAT) output_len += s->reg.f32 = gfx_printf("%f\r\n", s->reg.f32);
  return 0;
}
#endif

#ifndef STDF_IGN_EXCEPTION_HANDLER
int8_t exception_handler(State * s){
  fprintf(stderr, "Something went wrong\n");
  return 0;
}
#endif

#ifndef STDF_IGN_UNDEF
int8_t undef(State * s){
  fprintf(stderr, "Undefined function or variable: %s\n", (char *) s->_id);
  return 0;
}
#endif

#ifndef STDF_IGN_ERROR
int8_t f_error(State * s){
  fprintf(stderr, "Raising error\n");
  return JM_ERR0;
}
#endif

#ifndef STDF_IGN_QUIT
int8_t f_quit(State * s){
  fprintf(stderr, "Quit\n");
  stop = 1;
  return 0;
}
#endif

#ifndef STDF_IGN_INPUT
int8_t f_input(State * s){
  s->_m[0] = getc(stdin);
  return 0;
}
#endif

#define STAR_T_FUNCTIONS             \
  {(uint8_t*)"", exception_handler}, \
  {(uint8_t*)"PRINT", f_print},      \
  {(uint8_t*)"PC", f_print},         \
  {(uint8_t*)"PRINTSTR", f_printstr},\
  {(uint8_t*)"PS", f_printstr},      \
  {(uint8_t*)"PRINTNUM", f_printnum},\
  {(uint8_t*)"PN", f_printnum},      \
  {(uint8_t*)"QUIT", f_quit},        \
  {(uint8_t*)"ERROR", f_error},      \
  {(uint8_t*)"DT", f_printstr},      \
  {(uint8_t*)"WS", f_printstr},      \
  {(uint8_t*)"IN", f_input},         \
  {(uint8_t*)"INPUT", f_input},      \
  {NULL, undef}                      \

int8_t step_callback(State * s) {
  return 0;
}

Function ext[] = {
  STAR_T_FUNCTIONS,
};
State * s;

int8_t run(char * src) {
  s = (State*) malloc(sizeof(State));
  memset((void*)s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = st_run(s);
  return result;
}


void clean(void){
  st_state_free(s);
  s = NULL;
}

char src[] = "s\
LL^52!>L^!>\
CC^105!>C^!>\
f\
CX^>\
CY^>\
TX^>\
TY^>\
TZ^>\
X^>\
Y^>\
bI^\
sL 1- [\
  sCC;C! 1- [\
    b0I!\
    f0X!Y!\
    i0sCC;efCX!3/ i0sC;efCX@/ 2-\
    i0sLL;efCY!2/ i0sL;efCY@/ 1-\
    bI 10! [\
      f\
      X;TX!*\
      Y;TY!*\
      0TZ! TX;TZ+ TY;TZ+ 4?< (\
        x\
      )\
      Y 2*  X;Y*  CY;Y+\
      TX;X! TY;X- CX;X+\
      bI 1- ??\
    ]\
    bI;TZ! 38+ .\
    sC 1- ??\
  ]\
  b13TZ! .b10TZ! .\
  sL 1- ??\
]";

char last_key = 0;
char text[32] = {0};
char menu = 0;
extern uint32_t elm;
int running = 0;
int result = 0;
static uint64_t steps = 0;
uint32_t start = 0;
uint32_t elapsed = 0;
int frames = 0;
int compute_loops = 0;
int lowfps = 0;
int r = 57;
int g = 255;
int b = 20;

void gfx_app(int init) {
  if (init) {
    printf("App started\n");
  } else {
    printf("App stopped\n");
  }
}

void gfx_process_data(int delay) {
  for (int i = 0; running && i < (lowfps ? 85000 : 10000); i++) {
    State * sub = s;
    while (sub->sub) sub = sub->sub;
    steps++;
    int8_t r = st_step(s);
    if (r == LOOP_ST){
      running = 0;
      result = 0;
      elapsed = SDL_GetTicks() - start;
    } else if (r >= JM_ERR0 || r == BL_PREV){
      running = 0;
      result = r;
    }
  }
  compute_loops++;
  if (!running) {
    gfx_delay(delay);
  }
}

void gfx_draw(float fps) {
  if (running) {
    frames++;
  }

  gfx_set_color(255, 255, 255);
  sprintf(text, "steps: %"PRIu64, steps);
  gfx_text(text, 10, 35, 1);

  sprintf(text, "frames: %d", frames);
  gfx_text(text, 100, 35, 1);

  if (running) {
    sprintf(text, "Running...");
  } else {
    if (result == 0) {
      sprintf(text, "Done in %.2fs", elapsed / 1000.0);
    } else {
      sprintf(text, "Error: %d", result);
    }
  }
  gfx_text(text, 200, 35, 1);

  if (!running && !result) {
    sprintf(text, "fps: %.2f", frames / (elapsed / 1000.0));
    gfx_text(text, 300, 35, 1);
  }

  sprintf(text, lowfps ? "Low FPS" : "Normal FPS");
  gfx_text(text, 400, 35, 1);

  gfx_set_color(r, g, b);
  gfx_text(printf_buf, 8, 50, 1);

  gfx_set_color(255, 255, 255);
  gfx_text("\xae Press MENU + X to exit \xaf", 10, 10, 2);

  if (last_key != 0) {
    sprintf(text, "key: %d", last_key);
    gfx_text(text, 350, 10, 2);
  }
  sprintf(text, "%.1f fps | %.1fk draws ", fps, elm/1000.0);
  gfx_text(text, 480, 8, 1);
  sprintf(text, "compute %d | buffer %.1fk", compute_loops, printf_len/1000.0);
  gfx_text(text, 480, 18, 1);
  compute_loops = 0;
}

int gfx_on_key(char key, int down) {
  if (down) {
    printf("Key pressed: %d\n", key);
    beep(261, 50);
    if (key == BTN_MENU) {
      menu = 1;
    } else if (key == BTN_A) {
      if (running) {
        running = 0;
        result = 0;
        steps = 0;
        if (s) {
          clean();
        }
      }
      beep(440, 150);
      gfx_clear_text_buffer();
      start = SDL_GetTicks();
      s = (State*) malloc(sizeof(State));
      memset((void*)s, 0, sizeof(State));
      s->src = (uint8_t*) src;
      st_state_init(s);
      steps = 0;
      frames = 0;
      running = 1;
    } else if (key == BTN_B) {
      if (running) {
        running = 0;
        result = 0;
        steps = 0;
        if (s) {
          clean();
        }
      }
      beep(330, 150);
      gfx_clear_text_buffer();
      start = SDL_GetTicks();
      frames = 1;
      result = run(src);
      elapsed = SDL_GetTicks() - start;
    } else if (key == BTN_Y) {
      beep(240, 150);
      running = 0;
      result = 1;
      if (s) {
        clean();
      }
    } else if (key == BTN_X) {
      lowfps = !lowfps;
      if (lowfps) {
        beep(392, 150);
      } else {
        beep(523, 150);
      }
    } else if (key == BTN_L1) {
      r = rand() % 256;
      g = rand() % 256;
      b = rand() % 256;
    } else if (key == BTN_R1) {
      r = 57;
      g = 255;
      b = 20;
    } else if (key == BTN_L2) {
      r = 255;
      g = 0;
      b = 255;
    } else if (key == BTN_R2) {
      r = 255;
      g = 255;
      b = 255;
    }
    if (menu && key == BTN_X) {
      return 1;
    }
    last_key = key;
  } else {
    printf("Key released: %d\n", key);
    beep(392, 50);
    if (key == BTN_MENU) {
      menu = 0;
    }
    last_key = 0;
  }
  return 0;
}