#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <start_lang.h>
#include <debug_utils.h>
#include <lang_assertions.h>

Register RM;
uint8_t * M;
State * s;
char out[256] = "";
char input[256] = "";
char * input_ptr = input;
uint8_t stop = 0;
uint8_t force_debug = 0;
uint16_t output_len = 0;
extern uint64_t steps;

int8_t step_callback(State * s) {
  return debug_state(s, force_debug, 1) ? 1 : stop;
}

#ifdef ENABLE_FILES
char * load_file(const char * fname){
  char *source = NULL;
  FILE *fp = fopen(fname, "r");
  if (fp != NULL) {
    if (fseek(fp, 0L, SEEK_END) == 0) {
      long bufsize = ftell(fp);
      if (bufsize == -1) { return NULL; }
      source = malloc(sizeof(char) * (bufsize + 1));
      if (fseek(fp, 0L, SEEK_SET) != 0) { free(source); return NULL; }
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      source[newLen++] = '\0';
    }
    fclose(fp);
    return source;
  }
  return NULL;
}
#endif

int8_t run(char * src) {
  stop = 0;
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  memset(out, 0, 256);
  input_ptr = input;
  s->src = (uint8_t*) src;
  int8_t result = blockrun(s);
  force_debug = 0;
  RM.i8[0] = (s->_m - s->_m0 + 0) >= s->_mlen ? 123 : s->_m[0];
  RM.i8[1] = (s->_m - s->_m0 + 1) >= s->_mlen ? 123 : s->_m[1];
  RM.i8[2] = (s->_m - s->_m0 + 2) >= s->_mlen ? 123 : s->_m[2];
  RM.i8[3] = (s->_m - s->_m0 + 3) >= s->_mlen ? 123 : s->_m[3];
  M = s->_m0;
  memset(input, 0, 256);
  return result;
}

int8_t f_x(State * s){
  s->_m[0] = 123;
  return 0;
}

int8_t f_y(State * s){
  s->_m[0] = 100;
  return 0;
}

int8_t f_print(State * s){
  char tmp[2] = {(char) s->_m[0], 0};
  strcat(out, tmp);
#ifndef BENCHMARK
  printf(".");
#endif
  return 0;
}

int8_t f_input(State * s){
  s->_m[0] = *input_ptr;
  input_ptr++;
  return 0;
}

int8_t f_printnum(State * s){
  if (s->_type == INT8) s->reg.i8[0] = sprintf(out, "%u", s->reg.i8[0]);
  else if (s->_type == INT16) s->reg.i16[0] = sprintf(out, "%u", s->reg.i16[0]);
  else if (s->_type == INT32) s->reg.i32 = sprintf(out, "%u", s->reg.i32);
  else if (s->_type == FLOAT) s->reg.f32 = sprintf(out, "%.2f", s->reg.f32);
  return 0;
}

int8_t f_printstr(State * s){
  strcat(out, (char*) s->_m);
  return 0;
}

int8_t exception_handler(State * s){
  return 0;
}

int8_t undef(State * s){
  s->_m[0] = 111;
  return 0;
}

int8_t f_error(State * s){
  return JM_ERR0;
}

int8_t f_quit(State * s){
  stop = 1;
  return 0;
}

Function ext[] = {
  {(uint8_t*)"", exception_handler},
  {(uint8_t*)"X", f_x},
  {(uint8_t*)"Y", f_y},
  {(uint8_t*)"PRINT", f_print},
  {(uint8_t*)"PC", f_print},
  {(uint8_t*)"INPUT", f_input},
  {(uint8_t*)"IN", f_input},
  {(uint8_t*)"PRINTSTR", f_printstr},
  {(uint8_t*)"PRINTNUM", f_printnum},
  {(uint8_t*)"QUIT", f_quit},
  {(uint8_t*)"ERROR", f_error},
  {NULL, undef},
};

char * getBuffer(void){
  return out;
}

int clean(void){
  int ts = steps;
  steps = 0;
  free_memory(s);
  s = NULL;
  return ts;
}

void print_info(void){
  printf("state size: %lu\n", sizeof(State));
}

void validate(void){
  begin_section("NOP");
  assert_eq(run(" "), 0);
  assert_eq(run("   "), 0);
  assert_eq(run(" \x01 "), JM_PEXC);
  assert_eq(run(" ?\x01 "), JM_PEXC);
  end_section();

  begin_section("DATA TYPES");
  // default type is int8 and cleared reg
  assert_eq((run(""), s->_type), INT8);
  assert_eq((run(""), REG.f32), 0);
  // random assertions
  assert_eq((run("3"), s->reg.i8[0]), 3);
  assert_eq((run("1"), s->reg.i8[0]), 1);
  assert_eq((run("10"), s->reg.i8[0]), 10);
  assert_eq((run("20"), s->reg.i8[0]), 20);
  assert_eq((run("33"), s->reg.i8[0]), 33);
  assert_eq((run("128"), s->reg.i8[0]), 128);
  assert_eq((run("255"), s->reg.i8[0]), 255);
  assert_eq((run("256"), s->reg.i8[0]), 0);
  // integer overflow
  assert_eq((run("257"), s->reg.i8[0]), 1);
  assert_eq((run("257"), s->reg.i8[1]), 0);
  assert_eq((run("s65537"), s->reg.i16[0]), 1);
  assert_eq((run("s65537"), s->reg.i16[1]), 0);
  // type changes
  assert_eq((run("bs"), s->_type), INT16);
  assert_eq((run("bi"), s->_type), INT32);
  assert_eq((run("bf"), s->_type), FLOAT);
  assert_eq((run("ib"), s->_type), INT8);
  // random values
  assert_eq((run("s257"), s->reg.i16[0]), 257);
  assert_eq((run("i65537"), s->reg.i32), 65537);
  assert_eq((run("f16"), s->reg.f32), 16);
  assert_eq((run("f16@"), s->reg.f32), 0);
  assert_eq((run("f16@32"), s->reg.f32), 32);
  assert_eq((run("f16@32"), RM.f32), 16);
  end_section();

  begin_section("DATA ALIGNMENT");
  assert_eq((run("s257!0"), RM.i16[0]), 257);
  assert_eq((run(">s257!0"), RM.i16[0]), 257);
  assert_eq((run("2>s257!0"), RM.i16[0]), 257);
  assert_eq((run("3>s257!0"), RM.i16[0]), 257);
  assert_eq((run("i65537!0"), RM.i32), 65537);
  assert_eq((run(">i65537!0"), RM.i32), 65537);
  assert_eq((run("2>i65537!0"), RM.i32), 65537);
  assert_eq((run("3>i65537!0"), RM.i32), 65537);
  assert_eq((run("f3!2/0"), RM.f32), 3.0f/2.0f);
  assert_eq((run(">f3!2/0"), RM.f32), 3.0f/2.0f);
  assert_eq((run("2>f3!2/0"), RM.f32), 3.0f/2.0f);
  assert_eq((run("3>f3!2/0"), RM.f32), 3.0f/2.0f);
  end_section();

  begin_section("STORE");
  assert_eq((run("3!"), M[0]), 3);
  // store 3/2
  assert_eq((run("f3!2/"), RM.f32), 3.0f/2.0f);
  end_section();

  // tape-register notation
  // M[n-1] [REG|ANS> M[n]

  begin_section("Move/Load");
  // 3 [4|0> 4
  assert_eq((run("3!>4!"), M[0]), 3);
  assert_eq((run("3!>4!"), M[1]), 4);
  // [3|0> 3 4
  assert_eq((run(">4!<3!"), M[0]), 3);
  assert_eq((run(">4!<3!"), M[1]), 4);
  // [1|0> 3 4
  assert_eq((run(">4!<3!1"), M[0]), 3);
  assert_eq((run(">4!<3!1"), M[1]), 4);
  assert_eq((run(">4!<3!1"), s->reg.i8[0]), 1);
  // [3|0> 3 4
  assert_eq((run(">4!<3!1;"), M[0]), 3);
  // 0. [0|0>     - start
  // 1. ? [0|0>   - > right
  // 2. ? [4|0>   - 4 read const 4 into REG
  // 3. ? [4|0> 4 - ! store
  // 4. [4|0> ? 4 - < left
  // 5. [3|0> ? 4 - 3 read const 3 into REG
  // 6. [3|0> 3 4 - ! store
  // 7. [1|0> 3 4 - 1 read const 1 into REG
  // 8. 3 [1|0> 4 - < right
  // 9. 3 [4|0> 4 - ; load
  assert_eq((run(">4!<3!1>;"), RM.i8[0]), 4);
  assert_eq((run("s>4!<3!1>;"), RM.i8[0]), 4);
  assert_eq((run(""), s->_m - s->_m0), 0);
  assert_eq((run(">"), s->_m - s->_m0), 1);
  assert_eq((run(">>"), s->_m - s->_m0), 2);
  assert_eq((run("2>"), s->_m - s->_m0), 2);
  assert_eq((run(">>>"), s->_m - s->_m0), 3);
  assert_eq((run("3>"), s->_m - s->_m0), 3);
  assert_eq((run(">2>"), s->_m - s->_m0), 3);
  assert_eq((run(">>>>"), s->_m - s->_m0), 4);
  assert_eq((run("2>2>"), s->_m - s->_m0), 4);
  assert_eq((run("3>1>"), s->_m - s->_m0), 4);
  assert_eq((run("4>0>"), s->_m - s->_m0), 4);
  assert_eq((run("s>"), s->_m - s->_m0), 2);
  assert_eq((run("s1>"), s->_m - s->_m0), 2);
  assert_eq((run("s2>"), s->_m - s->_m0), 4);
  assert_eq((run("s3>"), s->_m - s->_m0), 6);
  assert_eq((run("i>"), s->_m - s->_m0), 4);
  assert_eq((run("i2>"), s->_m - s->_m0), 8);
  assert_eq((run("i3>"), s->_m - s->_m0), 12);
  assert_eq((run("f>"), s->_m - s->_m0), 4);
  assert_eq((run("f>>"), s->_m - s->_m0), 8);
  // undefined behavior: float constant is converted to int8 by truncation
  // assert_eq((run("f1>"), s->_m - s->_m0), 4);
  assert_eq((run("f1>"), s->_m - s->_m0), 0); // TODO: add warning
  // invalid moves
  assert_eq(run("<"), JM_REOB);
  assert_eq((run("s1000>"), 1000 > MEM_SIZE), 1);
  assert_eq(run("s1000>"), JM_REOB);
  end_section();

  begin_section("STRING");
  // register contains string length
  assert_eq((run("2!\"abc\""), s->reg.i8[0]), 3);
  // memory pointer stays the same
  assert_eq((run("2!\"abc\"3!"), M[0]), 3);
  assert_eq((run("2!\"abc\"3!"), M[1]), 'b');
  assert_eq((run("2!\"abc\"3!"), M[2]), 'c');
  assert_eq((run("2!\"abc\"3!"), M[3]), 0);
  // going foward to the end of the string using >
  assert_eq((run("2!\"abc\">3!"), M[0]), 'a');
  assert_eq((run("2!\"abc\">3!"), M[1]), 'b');
  assert_eq((run("2!\"abc\">3!"), M[2]), 'c');
  assert_eq((run("2!\"abc\">3!"), M[3]), 0);
  assert_eq((run("2!\"abc\">3!"), M[4]), 3);
  // escape
  assert_eq((run("2!\"a\\\"c\">3!"), s->reg.i8[0]), 3);
  assert_eq((run("2!\"a\\\"c\">3!"), M[0]), 'a');
  assert_eq((run("2!\"a\\\"c\">3!"), M[1]), '\"');
  assert_eq((run("2!\"a\\\"c\">3!"), M[2]), 'c');
  assert_eq((run("2!\"a\\\"c\">3!"), M[3]), 0);
  assert_eq((run("2!\"a\\\"c\">3!"), M[4]), 3);
  assert_eq((run("2!\"\\\"\">3!"), M[0]), '\"');
  assert_eq((run("2!\"\\\"\">3!"), M[1]), 0);
  assert_eq((run("2!\"\\\"\">3!"), M[2]), 3);
  // // ends with 0
  assert_eq((run("2!\"\""), M[0]), 0);
  end_section();

  begin_section("SWITCH");
  assert_eq((run("2"), s->reg.i8[0]), 2);
  assert_eq((run("2@"), M[0]), 2);
  assert_eq((run("2@"), s->reg.i8[0]), 0);
  assert_eq((run("2@3"), s->reg.i8[0]), 3);
  assert_eq((run("2@3"), M[0]), 2);
  end_section();

  begin_section("COMP");
  // [1|0> 0
  assert_eq((run("0@1?<"), s->_ans), 0);
  // [1|0> 1
  assert_eq((run("1@1?<"), s->_ans), 0);
  // [1|1> 2
  assert_eq((run("2@1?<"), s->_ans), 1);
  assert_eq((run("0@1@?>"), s->_ans), 0);
  assert_eq((run("1@1@?>"), s->_ans), 0);
  assert_eq((run("2@1@?>"), s->reg.i8[0]), 2);
  assert_eq((run("2@1@?>"), M[0]), 1);
  assert_eq((run("2@1@?>"), s->_ans), 1);
  assert_eq((run(""), s->_ans), 0);
  assert_eq((run("t"), s->_ans), 1);
  assert_eq((run("t~"), s->_ans), 0);
  assert_eq((run("3!;?="), s->_ans), 1);
  assert_eq((run("3!;?!"), s->_ans), 0);
  assert_eq((run("3!;?<"), s->_ans), 0);
  assert_eq((run("3!;?>"), s->_ans), 0);
  assert_eq((run("3!;?l"), s->_ans), 1);
  assert_eq((run("3!;?g"), s->_ans), 1);
  assert_eq((run("3!;??"), s->_ans), 1);
  assert_eq((run("3!;?z"), s->_ans), 0);
  assert_eq((run("s3!;?="), s->_ans), 1);
  assert_eq((run("s3!;?!"), s->_ans), 0);
  assert_eq((run("s3!;?<"), s->_ans), 0);
  assert_eq((run("s3!;?>"), s->_ans), 0);
  assert_eq((run("s3!;?l"), s->_ans), 1);
  assert_eq((run("s3!;?g"), s->_ans), 1);
  assert_eq((run("s3!;??"), s->_ans), 1);
  assert_eq((run("s3!;?z"), s->_ans), 0);
  assert_eq((run("i3!;?="), s->_ans), 1);
  assert_eq((run("i3!;?!"), s->_ans), 0);
  assert_eq((run("i3!;?<"), s->_ans), 0);
  assert_eq((run("i3!;?>"), s->_ans), 0);
  assert_eq((run("i3!;?l"), s->_ans), 1);
  assert_eq((run("i3!;?g"), s->_ans), 1);
  assert_eq((run("i3!;??"), s->_ans), 1);
  assert_eq((run("i3!;?z"), s->_ans), 0);
  assert_eq((run("f3!2/;?="), s->_ans), 1);
  assert_eq((run("f3!2/;?!"), s->_ans), 0);
  assert_eq((run("f3!2/;?<"), s->_ans), 0);
  assert_eq((run("f3!2/;?>"), s->_ans), 0);
  assert_eq((run("f3!2/;?l"), s->_ans), 1);
  assert_eq((run("f3!2/;?g"), s->_ans), 1);
  assert_eq((run("f3!2/;??"), s->_ans), 1);
  assert_eq((run("f3!2/;?z"), s->_ans), 0);
  end_section();

  begin_section("OP");
  // 0. [0|0> 0   - start
  // 1. [12|0> 0  - 12 read const 12 into REG
  // 2. [0|0> 12  - @  switch REG, MEM
  // 3. [1|0> 12  - 1  read const 1 into REG
  // 4. [1|0> 13  - +  MEM op REG -> MEM
  assert_eq((run("12@1+"), RM.i8[0]), 13);
  assert_eq((run("s12@1+"), RM.i16[0]), 13);
  assert_eq((run("f12@1+"), RM.f32), 13);
  assert_eq((run("12@1-"), RM.i8[0]), 11);
  assert_eq((run("12@1--"), RM.i8[0]), 10);
  assert_eq((run("s12@1--"), RM.i16[0]), 10);
  assert_eq((run("f12@1--"), RM.f32), 10);
  assert_eq((run("2@3*"), RM.i8[0]), 6);
  assert_eq((run("s2@3*"), RM.i16[0]), 6);
  assert_eq((run("i2@3*"), RM.i32), 6);
  assert_eq((run("f2@3*"), RM.f32), 6);
  assert_eq((run("8@2/"), RM.i8[0]), 4);
  assert_eq((run("s8@2/"), RM.i16[0]), 4);
  assert_eq((run("i8@2/"), RM.i32), 4);
  assert_eq((run("s9@6%"), RM.i16[0]), 3);
  assert_eq((run("i9@6%"), RM.i32), 3);
  assert_eq((run("12!1+"), RM.i8[0]), 13);
  assert_eq((run("12!1-"), RM.i8[0]), 11);
  assert_eq((run("12!1--"), RM.i8[0]), 10);
  assert_eq((run("2!3*"), RM.i8[0]), 6);
  assert_eq((run("8!2/"), RM.i8[0]), 4);
  assert_eq((run("s2!1&;"), RM.i16[0]), 0);
  assert_eq((run("i2!1&;"), RM.i32), 0);
  assert_eq((run("s2!1|;"), RM.i16[0]), 3);
  assert_eq((run("i2!1|;"), RM.i32), 3);
  assert_eq((run("s2!1w^;"), RM.i16[0]), 3);
  assert_eq((run("i2!1w^;"), RM.i32), 3);
  assert_eq((run("i0w~"), s->reg.i8[0]), 255);
  assert_eq((run("i0w~"), s->reg.i8[1]), 255);
  assert_eq((run("i0w~"), s->reg.i8[2]), 255);
  assert_eq((run("i0w~"), s->reg.i8[3]), 255);
  end_section();

  begin_section("IF");
  // valid empty expressions
  assert_eq(run("()"), 0);
  assert_eq(run("(:)"), 0);
  // Ans is false by default
  assert_eq((run(""), s->_ans), 0);
  assert_eq(run("1(2)"), 0);
  assert_eq((run("1(2)"), s->reg.i8[0]), 1);
  assert_eq((run("1(2:)"), s->reg.i8[0]), 1);
  assert_eq((run("1(:2)"), s->reg.i8[0]), 2);
  // false
  assert_eq(run("0?!(1:2)"), 0);
  assert_eq((run("0?!(1:2)"), s->reg.i8[0]), 2);
  assert_eq((run("(2:3)"), s->reg.i8[0]), 3);
  // true
  assert_eq(run("0?=(1:2)"), 0);
  assert_eq((run("0?=(1:2)"), s->_ans), 1);
  assert_eq((run("0?=(1:2)"), s->reg.i8[0]), 1);
  // not
  assert_eq((run("0?=~(1:2)"), s->_ans), 0);
  assert_eq((run("0?=~(1:2)"), s->reg.i8[0]), 2);
  // 0 > 1 ?
  assert_eq((run("0@1@?>(2:3)!"), s->_m[0]), 3);
  // 1 > 1 ?
  assert_eq((run("1@1@?>(2:3)!"), s->_m[0]), 3);
  // 2 > 1 ?
  assert_eq((run("2@1@?>(2:3)!"), s->_m[0]), 2);
  end_section();

  begin_section("Stack");
  assert_eq((run("b3p4p"), M[0]), 3);
  assert_eq((run("b3p4p"), M[1]), 4);
  assert_eq((run("b3p4ph"), s->reg.i16[0]), 2);
  assert_eq(run("b3p4po"), JM_EXEN);
  assert_eq((run("b3p4po "), s->reg.i8[0]), 4);
  assert_eq(run("b3p4poo"), JM_EXEN);
  assert_eq((run("b3p4poo "), s->reg.i8[0]), 3);
  assert_eq((run("b3p4pooh"), s->reg.i16[0]), 0);
  assert_eq((run("b1p2p3p4p5p 0 o+o+o+o+o+"), RM.i8[0]), 15);
  assert_eq((run("b1p2p3p4p5p 0 t[ o+ ?h]"), RM.i8[0]), 15);
  end_section();

  begin_section("WHILE");
  assert_eq(run("[]"), 0);
  assert_eq(run("1[2]"), 0);
  assert_eq((run("1[2]"), s->reg.i8[0]), 1);
  assert_eq(run("0?=[x]"), 0);
  assert_eq(run("1[2x3]"), 0);
  assert_eq((run("1[2x3]"), s->reg.i8[0]), 1);
  assert_eq(run("0?=1[2x3]"), 0);
  assert_eq((run("0?=1[2x3]"), s->reg.i8[0]), 2);
  assert_eq(run("1?![x]"), 0);
  assert_eq((run("1?![x]"), s->reg.i8[0]), 1);
  assert_eq(run("1@1?![-?!]"), 0);
  assert_eq((run("12@?![1-?!]"), RM.i8[0]), 1);
  assert_eq((run("12@?![1-?!]"), s->reg.i8[0]), 1);
  assert_eq((run("8!?=[1-?! c x ]"), RM.i8[0]), 1);
  assert_eq((run("1! t[ 2 t[ 3 x 4! ] 5 ?=]"), s->reg.i8[0]), 5);
  assert_eq((run("1! t[ 2 t[ 3 x 4! ] 5 ?=]"), RM.i8[0]), 1);
  assert_eq((run("1! t[ 2 t[ 3 (x:c) 4! t] 5 ?=]"), s->reg.i8[0]), 5);
  assert_eq((run("0! t[ t[7?=(0! x :1+) 3 t] 5 ??]"), s->reg.i8[0]), 5);
  assert_eq((run("t[t[1?=(c:(c:x))]]"), REG.i8[0]), 1);
  assert_eq((run("t[t[1?=(c:(c:2x))]]"), REG.i8[0]), 2);
  assert_eq((run("t[t[1?=(c:(c:x))]3]"), REG.i8[0]), 3);
  assert_eq((run("t[t[1?=(c:(c:x))]]4"), REG.i8[0]), 4);
  end_section();

  begin_section("Fibonacci");
  assert_eq((run("8!>0!>1!?=[2<1-?!2>;<@>+]"), RM.i8[0]), 21);
  assert_eq((run("8!>0!>1!?=[2<1-?!2>;<@>+]"), RM.i8[0]), 21);
  assert_eq((run("9!>0!>1!?=[2<1-?!2>;<@>+]"), RM.i8[0]), 34);
  assert_eq((run("10!>0!>1!?=[2<1-?!2>;<@>+]"), RM.i8[0]), 55);
  assert_eq((run("i46!>0!>1!?=[2<1-?!2>;<@>+]"), RM.i32), 1836311903);
  end_section();

  begin_section("blockrun");
  char * code = "i46!>0!>1!?=[2<1-?!2>;<@>+]";
  char * block = NULL;
  for (int max_block_size = 1; max_block_size <= strlen(code); max_block_size++){
    int direction = 0;
    int index = 0;
    s = (State*) malloc(sizeof(State));
    memset(s, 0, sizeof(State));
    block = realloc(block, (max_block_size+1)*sizeof(char));
    while (1) {
      int block_size = max_block_size;
      s->_ignend = 1;
      if (index*max_block_size + max_block_size >= strlen(code)){
        block_size = strlen(code) - index*max_block_size;
        s->_ignend = 0;
      }
      if (block_size <= 0){
        break;
      }
      strncpy(block, code + (index*max_block_size),  block_size);
      block[block_size] = '\0';
      s->src = (uint8_t*) block;
      direction = blockrun(s);
      // printf("%s %d %d %d\n", block, block_size, direction, index);

      if (direction == -1){
        index--;
      } else if (direction == 0){
        index++;
      } else {
        //error
        printf("ss%d\n", direction);
        break;
      }
    }
    assert_eq(RM.i32, 1836311903);
    assert_eq(direction == -1 || direction == 0, 1);
  }
  free(block);
  end_section();

  begin_section("IDS");
  assert_eq((run("ABC^1!>2!>3!ABC;"), s->reg.i8[0]), 1);
  assert_eq((run("N^8!>B^0!>A^1!?=[N 1-?!A;B@A+]"), RM.i8[0]), 21);
  assert_eq((run("N^>B^0!>A^1!N 8!?=[N 1-?!A;B@A+]"), RM.i8[0]), 21);
  assert_eq((run("A^0!>2!A;"), RM.i8[0]), 0);
  assert_eq((run("A^0!>2!A;>A^ A;"), RM.i8[0]), 2);
  end_section();

  begin_section("Functions");
  assert_eq((run("X"), M[0]), 123);
  assert_eq((run("Y"), M[0]), 100);
  assert_eq((run("X Y"), M[0]), 100);
  // undef
  assert_eq((run("X Y Z"), M[0]), 111);
  // return
  assert_eq((run("32 QUIT 64"), s->reg.i8[0]), 32);
  assert_eq((run("X QUIT"), M[0]), 123);
  assert_eq(run("ERROR"), JM_ERR0);
  end_section();

  begin_section("Functions2");
  assert_eq((run("Z{1!}2!Z"), M[0]), 1);
  assert_eq((run("Z{1!}2!>Z"), M[0]), 2);
  assert_eq((run("Z{1!}2!>Z"), M[1]), 1);
  // inner undef
  assert_eq((run("Z{1!K }2!>Z"), M[1]), 111);
  assert_eq((run("Z{K{1!}K }2!>Z"), M[1]), 1);
  assert_eq((run("FIB{;>0!>1!?=[2<1-?!2>;<@>+]}8!FIB 2>;"), RM.i32), 21);
  assert_eq((run("FIB{;>0!>1!?=[2<1-?!2>;<@>+]}3!FIB 8!FIB 2>;"), RM.i32), 21);
  assert_eq((run("iFIB{i;>0!>1!?=[2<1-?!2>;<@>+]}46!FIB 2>;"), RM.i32), 1836311903);
  end_section();

  begin_section("Run");
  assert_eq((run("\"1!\"#"), M[0]), 1);
  assert_eq((run("\"\\\"1!\\\"#\"#"), M[0]), 1);
  assert_eq((run("\"\\\"\\\\\"1!\\\\\"#\\\"#\"#"), M[0]), 1);
  assert_eq((run("\"\\\"\\\\\"\\\\\\\"1!\\\\\\\"#\\\\\"#\\\"#\"#"), M[0]), 1);
  {
    char src[256] = "";
    int max = 20;
    int i = 4;
    while(++i <= max && 6 + 2 + 2*i + i * i < 256){
      strcat(src, "s256mb");
      for (int j = 0; j < i; j++){
        for (int k = 0; k < j; k++){
          strcat(src, "\\");
        }
        strcat(src, "\"");
      }
      strcat(src, "1!");
      for (int j = i - 1; j > 0; j--){
        for (int k = 0; k < j; k++){
          strcat(src, "\\");
        }
        strcat(src, "\"#");
      }
      strcat(src, "\"#");
      assert_eq((run(src), M[0]), 1);
      src[0] = 0;
    }
  }
  assert_eq((run("\"1!>2!>3!\"#"), M[0]), 1);
  assert_eq((run("\"Z{1!K}2!>Z\"#"), M[1]), 111);
  assert_eq((run("\"Z{1!K}2!\"#>Z"), M[1]), 111);
  end_section();

  begin_section("Return");
  assert_eq((run("r1!"), M[0]), 0);
  assert_eq((run("1!r2!"), M[0]), 1);
  assert_eq((run("Z{11!rK}2!>Z"), M[1]), 11);
  assert_eq((run("\"Z{11!rK}2!\"#>Z"), M[1]), 11);
  end_section();

  begin_section("IO");
  assert_str_eq((run("32!."), out), " ");
  assert_str_eq((run("\"Hello\"[.>]"), out), "Hello");
  strcat(input, "Hello");
  assert_str_eq((run(",[>,]"), (char *)s->_m0), "Hello");
  strcat(input, "123");
  assert_str_eq((run("34!>,[>,]34!"), (char *)s->_m0), "\"123\"");
  strcat(input, "123");
  assert_eq((run("A^,[>,]33!A#"), M[0]), 123);
  strcat(input, "123");
  assert_eq((run("I{A^,[>,]33!A#}I"), M[0]), 123);
  strcat(input, "123\n231");
  assert_eq((run("I{A^,[>,10?!(0?!)]33!A#}I>I"), M[0]), 123);
  strcat(input, "123\n231");
  assert_eq((run("I{A^,[>,10?!(0?!)]33!A#}I>I"), M[1]), 231);
  end_section();

  #ifdef ENABLE_FILES
  #ifndef BENCHMARK
  #ifdef LONG_TEST
  begin_section("Pi");
  {
    char * src = load_file("tests/bf/pi.st");
    strcat(input, "\x05");
    assert_eq(run(src), 0);
    char pi[] = "3.1415";
    assert_str_eq(out, pi);
    free(src);
  }
  end_section();
  begin_section("Pi_old");
  {
    char * src = load_file("tests/bf/pi_old.st");
    assert_eq(run(src), 0);
    char pi[] = "3.1415";
    assert_str_eq(out, pi);
    free(src);
  }
  end_section();

  #ifdef MAX_ITERATION_COUNT
  begin_section("e");
  {
    memset(out, 0, 256);
    char * src = load_file("tests/e.st");
    assert_eq(run(src), 0);
    free(src);
  }
  end_section();
  #endif
  #endif

  begin_section("Quine");
  {
    char * src = load_file("tests/quine.st");
    assert_str_eq(src, (run(src), out));
    free(src);
  }
  end_section();

  begin_section("Calc");
  {
    strcat(input, "22+62");
    char * src = load_file("tests/bf/calc.st");
    assert_str_eq((run(src), out), "84");
    free(src);
  }
  end_section();
  #endif
  #endif

  begin_section("Malloc");
  assert_eq((run("4m"), s->_mlen), 4);
  assert_eq((run("20m"), s->_mlen), 20);
  // memory overflow
  assert_eq((run("5m"), s->_mlen), 5);
  assert_eq(run("5m\"123\""), 0);
  assert_eq(run("5m\"1234\""), 0);
  assert_eq(run("5m\"12345\""), JM_REOB);
  assert_eq(run("5m\"123456\""), JM_REOB);
  assert_eq(run("7m\"123456\""), 0);
  end_section();

  begin_section("Goto zero");
  assert_eq((run("1!z"), s->_m - s->_m0), 1);
  assert_eq((run("1!>1!>1!2<z"), s->_m - s->_m0), 3);
  end_section();

  begin_section("BITWISE");
  assert_eq((run("2!3w<"), RM.i8[0]), 16);
  assert_eq((run("16!3w>"), RM.i8[0]), 2);
  assert_eq((run("15!4w< 15|"), RM.i8[0]), 255);
  assert_eq((run("s255!8w< 255|"), RM.i8[0]), 255);
  assert_eq((run("s255!8w< 255|"), RM.i8[1]), 255);
  assert_eq((run("s255!8w< 255|"), RM.i16[0]), 65535);
  assert_eq((run("i255!8w< 255|8w< 255|8w< 255|"), RM.i8[0]), 255);
  assert_eq((run("i255!8w< 255|8w< 255|8w< 255|"), RM.i8[1]), 255);
  assert_eq((run("i255!8w< 255|8w< 255|8w< 255|"), RM.i8[2]), 255);
  assert_eq((run("i255!8w< 255|8w< 255|8w< 255|"), RM.i8[3]), 255);
  assert_eq((run("s256!1w>"), RM.i8[0]), 128);
  assert_eq((run("i256!1w>"), RM.i8[0]), 128);
  assert_eq((run("2!1w&"), RM.i8[0]), 0);
  assert_eq((run("2!1w|"), RM.i8[0]), 3);
  assert_eq((run("3!1w^"), RM.i8[0]), 2);
  assert_str_eq((run("255 PRINTNUM"), out), "255");
  assert_str_eq((run("255w~ PRINTNUM"), out), "0");
  assert_str_eq((run("0w~ PRINTNUM"), out), "255");
  assert_str_eq((run("128w~ PRINTNUM"), out), "127");
  assert_str_eq((run("s0w~ PRINTNUM"), out), "65535");
  assert_eq((run("3w~"), RM.i8[0]), 0); // TODO: fix microcuts int conversion
  end_section();

  begin_section("TYPE_CASTING");
  assert_str_eq((run("f3!2/ ; PRINTNUM"), out), "1.50");
  //float to int
  assert_str_eq((run("f3!2/ ;i PRINTNUM"), out), "1");
  //float to int to float
  assert_str_eq((run("f3!2/ ;if PRINTNUM"), out), "1.00");
  //byte to int to float
  assert_str_eq((run("b32 if PRINTNUM"), out), "32.00");
  end_section();

  begin_section("ROTATE REGISTER");
  assert_eq((run("3k"), REG.i8[0]), 0);
  assert_eq((run("3k"), REG.i8[1]), 3);
  assert_eq((run("3kk"), REG.i8[2]), 3);
  assert_eq((run("3kkk"), REG.i8[3]), 3);
  assert_eq((run("3kkkk"), REG.i8[0]), 3);
  assert_eq((run("1k"), REG.i16[0]), 256);
  assert_eq((run("1k1"), REG.i16[0]), 257);
  assert_eq((run("1k1"), REG.i32), 257);
  assert_eq((run("1kk1"), REG.i32), 65537);
  assert_eq((run("1kkk1"), REG.i32), 16777217);
  assert_eq((run("1kkkk12"), REG.i32), 12);
  assert_eq((run("s300kk200"), REG.i16[0]), 200);
  assert_eq((run("s300kk200"), REG.i16[1]), 300);
  assert_eq((run("sA^300!>B^200! A;kkB; "), REG.i16[0]), 200);
  assert_eq((run("sA^300!>B^200! A;kkB; "), REG.i16[1]), 300);
  assert_eq((run("sA^300!>B^200! T^ A;kkB; "), REG.i16[0]), 200);
  assert_eq((run("sA^300!>B^200! T^ A;kkB; "), REG.i16[1]), 300);
  end_section();

  begin_section("BF-LIKE");
  //+++[-]
  assert_eq((run("+++??[-??]"),  M[0]), 0);
  //+++
  assert_eq((run("+++"), M[0]), 3);
  //+++++[>>+<<-] copy value to 2 cells to the right
  assert_eq((run("+++++??[>>+<<-??]"), M[2]), 5);
  //+++++>+++[<+>-] addition 5 + 3
  assert_eq((run("+++++>+++??[<+>-??]"), M[0]), 8);
  //+++++++>+++++[<-] subtraction 7 - 2
  assert_eq((run("+++++++>+++++??[<->-??]"), M[0]), 2);
  //+++[>+++++<-] multiplication 3 * 5
  assert_eq((run("+++??[>+++++<-??]"), M[1]), 15);
  //+[>>+>+<<<-]>>>[<<<+>>>-]>+<<[-----[>]>>[<<<+++>>>[-]] if(x == 5){y = 3}
  assert_eq((run("+++++??[>>+>+<<<-??]>>>??[<<<+>>>-??]>+<<??[-----??[>??]>>??[<<<+++>>>??[-??]??]"), M[1]), 3);
  assert_eq((run("++++??[>>+>+<<<-??]>>>??[<<<+>>>-??]>+<<??[-----??[>??]>>??[<<<+++>>>??[-??]??]"), M[1]), 0);
  assert_eq((run("++++++??[>>+>+<<<-??]>>>??[<<<+>>>-??]>+<<??[-----??[>??]>>??[<<<+++>>>??[-??]??]"), M[1]), 0);
  end_section();

  begin_section("BF");
  assert_eq((run("+++[-]"),  M[0]), 0);
  assert_eq((run("+++"), M[0]), 3);
  assert_eq((run("+++++[>>+<<-]"), M[2]), 5);
  assert_eq((run("+++++>+++[<+>-]"), M[0]), 8);
  assert_eq((run("+++++++>+++++[<->-]"), M[0]), 2);
  assert_eq((run("+++[>+++++<-]"), M[1]), 15);
  assert_eq((run("+++++[>>+>+<<<-]>>>[<<<+>>>-]>+<<[-----[>]>>[<<<+++>>>[-]]"), M[1]), 3);
  assert_eq((run("++++[>>+>+<<<-]>>>[<<<+>>>-]>+<<[-----[>]>>[<<<+++>>>[-]]"), M[1]), 0);
  assert_eq((run("++++++[>>+>+<<<-]>>>[<<<+>>>-]>+<<[-----[>]>>[<<<+++>>>[-]]"), M[1]), 0);
  strcat(input, "Hello");
  assert_str_eq((run(">,[>,]<[.<]"), out), "olleH");
  end_section();
}
