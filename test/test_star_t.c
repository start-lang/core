#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <star_t.h>
#include <time.h>

uint8_t ** M;
State * s;
char out[256] = "";
double time_spent;

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

int8_t run(char * src) {
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  memset(out, 0, 256);
  s->src = (uint8_t*) src;
  clock_t begin = clock();
  int8_t result = blockrun(s, 1);
  clock_t end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  // if (s->_ic > 100){
  //   printf("%d %d IPS\n", s->_ic, (int)(s->_ic/time_spent));
  // }
  M = &(s->_m0);
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
  printf(".");
  return 0;
}

int8_t f_printstr(State * s){
  strcat(out, (char*) s->_m);
  return 0;
}

int8_t exceptionHandler(State * s){
  return 0;
}

int8_t undef(State * s){
  s->_m[0] = 111;
  return 0;
}

Function ext[] = {
  {(uint8_t*)"", exceptionHandler},
  {(uint8_t*)"X", f_x}, 
  {(uint8_t*)"Y", f_y}, 
  {(uint8_t*)"PRINT", f_print}, 
  {(uint8_t*)"PRINTSTR", f_printstr}, 
  {NULL, undef},
};

char * getBuffer(){
  return out;
}

void clean(){
  free_memory(s);
}

int main(void){
  setvbuf(stdout, NULL, _IONBF, 0);
  set_cleanup(clean);

  begin_section("NOP");
  assert_eq(run(" "), 0);
  assert_eq(run("   "), 0);
  // assert_eq(run(" ` "), -1);
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

  begin_section("STORE");
  assert_eq((run("3!"), (*M)[0]), 3);
  // store 3/2
  assert_eq((run("f3!2/"), RM.f32), 3.0f/2.0f);
  end_section();

  // tape-register notation
  // M[n-1] [REG|ANS> M[n]

  begin_section("Move/Load");
  // 3 [4|0> 4
  assert_eq((run("3!>4!"), (*M)[0]), 3);
  assert_eq((run("3!>4!"), (*M)[1]), 4);
  // [3|0> 3 4
  assert_eq((run(">4!<3!"), (*M)[0]), 3);
  assert_eq((run(">4!<3!"), (*M)[1]), 4);
  // [1|0> 3 4
  assert_eq((run(">4!<3!1"), (*M)[0]), 3);
  assert_eq((run(">4!<3!1"), (*M)[1]), 4);
  assert_eq((run(">4!<3!1"), s->reg.i8[0]), 1);
  // [3|0> 3 4
  assert_eq((run(">4!<3!1;"), RM.i8[0]), 3);
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
  end_section();

  begin_section("STRING");
  // register contains string length
  assert_eq((run("2!\"abc\""), s->reg.i8[0]), 3);
  // memory pointer stays the same
  assert_eq((run("2!\"abc\"3!"), (*M)[0]), 3);
  assert_eq((run("2!\"abc\"3!"), (*M)[1]), 'b');
  assert_eq((run("2!\"abc\"3!"), (*M)[2]), 'c');
  assert_eq((run("2!\"abc\"3!"), (*M)[3]), 0);
  // going foward to the end of the string using >
  assert_eq((run("2!\"abc\">3!"), (*M)[0]), 'a');
  assert_eq((run("2!\"abc\">3!"), (*M)[1]), 'b');
  assert_eq((run("2!\"abc\">3!"), (*M)[2]), 'c');
  assert_eq((run("2!\"abc\">3!"), (*M)[3]), 0);
  assert_eq((run("2!\"abc\">3!"), (*M)[4]), 3);
  // escape
  assert_eq((run("2!\"a\\\"c\">3!"), s->reg.i8[0]), 3);
  assert_eq((run("2!\"a\\\"c\">3!"), (*M)[0]), 'a');
  assert_eq((run("2!\"a\\\"c\">3!"), (*M)[1]), '\"');
  assert_eq((run("2!\"a\\\"c\">3!"), (*M)[2]), 'c');
  assert_eq((run("2!\"a\\\"c\">3!"), (*M)[3]), 0);
  assert_eq((run("2!\"a\\\"c\">3!"), (*M)[4]), 3);
  assert_eq((run("2!\"\\\"\">3!"), (*M)[0]), '\"');
  assert_eq((run("2!\"\\\"\">3!"), (*M)[1]), 0);
  assert_eq((run("2!\"\\\"\">3!"), (*M)[2]), 3);
  // // ends with 0
  assert_eq((run("2!\"\""), (*M)[0]), 0);
  end_section();

  begin_section("SWITCH");
  assert_eq((run("2"), s->reg.i8[0]), 2);
  assert_eq((run("2@"), (*M)[0]), 2);
  assert_eq((run("2@"), s->reg.i8[0]), 0);
  assert_eq((run("2@3"), s->reg.i8[0]), 3);
  assert_eq((run("2@3"), (*M)[0]), 2);
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
  assert_eq((run("2@1@?>"), (*M)[0]), 1);
  assert_eq((run("2@1@?>"), s->_ans), 1);
  end_section();

  begin_section("OP");
  // 0. [0|0> 0   - start
  // 1. [12|0> 0  - 12 read const 12 into REG
  // 2. [0|0> 12  - @  switch REG, MEM
  // 3. [1|0> 12  - 1  read const 1 into REG
  // 4. [1|0> 13  - +  MEM op REG -> MEM
  assert_eq((run("12@1+"), RM.i8[0]), 13);
  assert_eq((run("12@1-"), RM.i8[0]), 11);
  assert_eq((run("12@1--"), RM.i8[0]), 10);
  assert_eq((run("2@3*"), RM.i8[0]), 6);
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
  assert_eq(run("?!(1:2)"), 0);
  assert_eq((run("?!(1:2)"), s->reg.i8[0]), 2);
  assert_eq((run("(2:3)"), s->reg.i8[0]), 3);
  // true
  assert_eq(run("?=(1:2)"), 0);
  assert_eq((run("?=(1:2)"), s->_ans), 1);
  assert_eq((run("?=(1:2)"), s->reg.i8[0]), 1);
  // 0 > 1 ?
  assert_eq((run("0@1@?>(2:3)!"), s->_m[0]), 3);
  // 1 > 1 ?
  assert_eq((run("1@1@?>(2:3)!"), s->_m[0]), 3);
  // 2 > 1 ?
  assert_eq((run("2@1@?>(2:3)!"), s->_m[0]), 2);
  end_section();

  begin_section("Stack");
  assert_eq((run("b3p4p"), (*M)[0]), 3);
  assert_eq((run("b3p4p"), (*M)[1]), 4);
  assert_eq((run("b3p4ph"), s->reg.i16[0]), 2);
  assert_eq((run("b3p4po"), s->reg.i8[0]), 4);
  assert_eq((run("b3p4poo"), s->reg.i8[0]), 3);
  assert_eq((run("b3p4pooh"), s->reg.i16[0]), 0);
  assert_eq((run("b1p2p3p4p5p o+o+o+o+"), RM.i8[0]), 15);
  assert_eq((run("b1p2p3p4p5p ?=[ <+; ?h]"), RM.i8[0]), 15);
  assert_eq((run("b1p2p3p4p5p ?=[ o+ ?h]"), RM.i8[0]), 15);
  end_section();

  begin_section("WHILE");
  assert_eq(run("[]"), 0);
  assert_eq(run("1[2]"), 0);
  assert_eq((run("1[2]"), s->reg.i8[0]), 1);
  assert_eq(run("?=[x]"), 0);
  assert_eq(run("1[2x3]"), 0);
  assert_eq((run("1[2x3]"), s->reg.i8[0]), 1);
  assert_eq(run("?=1[2x3]"), 0);
  assert_eq((run("?=1[2x3]"), s->reg.i8[0]), 2);
  assert_eq(run("1?![x]"), 0);
  assert_eq((run("1?![x]"), s->reg.i8[0]), 1);
  assert_eq(run("1@1?![-?!]"), 0);
  assert_eq((run("12@?![1-?!]"), RM.i8[0]), 1);
  assert_eq((run("12@?![1-?!]"), s->reg.i8[0]), 1);
  assert_eq((run("8!?=[1-?! c x ]"), RM.i8[0]), 1);
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
      if (index*max_block_size + max_block_size >= strlen(code)){
        block_size = strlen(code) - index*max_block_size;
      }
      if (block_size <= 0){
        break;
      }
      strncpy(block, code + (index*max_block_size),  block_size);
      block[block_size] = '\0';
      s->src = (uint8_t*) block;
      direction = blockrun(s, 0);
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
  }
  free(block);
  end_section();

  begin_section("IDS");
  assert_eq((run("ABC^1!>2!>3!ABC;"), s->reg.i8[0]), 1);
  assert_eq((run("N^8!>B^0!>A^1!?=[N1-?!A;B@A+]"), RM.i8[0]), 21);
  assert_eq((run("N^>B^0!>A^1!N8!?=[N1-?!A;B@A+]"), RM.i8[0]), 21);
  end_section();

  begin_section("Functions");
  assert_eq((run("X"), (*M)[0]), 123);
  assert_eq((run("Y"), (*M)[0]), 100);
  assert_eq((run("X Y"), (*M)[0]), 100);
  // undef
  assert_eq((run("X Y Z"), (*M)[0]), 111);
  end_section();

  begin_section("Functions2");
  assert_eq((run("Z{1!}2!Z"), (*M)[0]), 1);
  assert_eq((run("Z{1!}2!>Z"), (*M)[0]), 2);
  assert_eq((run("Z{1!}2!>Z"), (*M)[1]), 1);
  // inner undef
  assert_eq((run("Z{1!K }2!>Z"), (*M)[1]), 111);
  assert_eq((run("Z{K{1!}K }2!>Z"), (*M)[1]), 1);
  assert_eq((run("FIB{;>0!>1!?=[2<1-?!2>;<@>+]}8!FIB2>;"), RM.i32), 21);
  assert_eq((run("iFIB{i;>0!>1!?=[2<1-?!2>;<@>+]}46!FIB2>;"), RM.i32), 1836311903);
  end_section();

  begin_section("Run");
  assert_eq((run("\"1!\"#"), (*M)[0]), 1);
  assert_eq((run("\"Z{1!K}2!>Z\"#"), (*M)[1]), 111);
  assert_eq((run("\"Z{1!K}2!\"#>Z"), (*M)[1]), 111);
  end_section();

  begin_section("Return");
  assert_eq((run("r1!"), (*M)[0]), 0);
  assert_eq((run("1!r2!"), (*M)[0]), 1);
  assert_eq((run("Z{11!rK}2!>Z"), (*M)[1]), 11);
  assert_eq((run("\"Z{11!rK}2!\"#>Z"), (*M)[1]), 11);
  end_section();

  begin_section("Pi");
  {
    char * src = load_file("test/pi.st"); 
    char pi[] = "3.141592653";
    assert_str_eq(pi, (run(src), out));
    free(src);
  }
  end_section();

  begin_section("Quine");
  {
    char * src = load_file("test/quine.st"); 
    assert_str_eq(src, (run(src), out));
    free(src);
  }
  end_section();

  // begin_section("e");
  // {
  //   memset(out, 0, 256);
  //   char * src = load_file("test/e.st"); 
  //   assert_eq(run(src), 0);
  //   free(src);
  // }
  // end_section();

  begin_section("Malloc");
  assert_eq((run("2m"), s->_mlen), 2);
  assert_eq((run("20m"), s->_mlen), 20);
  end_section();

  begin_section("Goto zero");
  assert_eq((run("1!z"), s->_m - s->_m0), 1);
  assert_eq((run("1!>1!>1!2<z"), s->_m - s->_m0), 3);
  end_section();
  
  // end_tests();

  printf("state size: %ld\n", sizeof(State));

  return 0;
}
