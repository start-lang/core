#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <star_t.h>

uint8_t ** M;
State * s;

int8_t run(char * src) {
  s = (State*) malloc(sizeof(State));
  memset(s, 0, sizeof(State));
  s->src = (uint8_t*) src;
  int8_t result = blockrun(s);
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
  {NULL, undef},
};

void clean(){
  free_memory(s);
}

int main(void){

  set_cleanup(clean);

  begin_section("NOP");
  assert_eq(run(" "), 0);
  assert_eq(run("   "), 0);
  // assert_eq(run(" ` "), -1);
  end_section();

  begin_section("DATA TYPES");
  // default type is int8 and cleared regs
  assert_eq((run(""), s->_type), INT8);
  assert_eq((run(""), s->a.f32), 0);
  assert_eq((run(""), s->b.f32), 0);
  // random assertions
  assert_eq((run("3"), s->a.i8[0]), 3);
  assert_eq((run("1"), s->a.i8[0]), 1);
  assert_eq((run("10"), s->a.i8[0]), 10);
  assert_eq((run("20"), s->a.i8[0]), 20);
  assert_eq((run("33"), s->a.i8[0]), 33);
  assert_eq((run("128"), s->a.i8[0]), 128);
  assert_eq((run("255"), s->a.i8[0]), 255);
  assert_eq((run("256"), s->a.i8[0]), 0);
  // integer overflow
  assert_eq((run("257"), s->a.i8[0]), 1);
  assert_eq((run("257"), s->a.i8[1]), 0);
  assert_eq((run("s65537"), s->a.i16[0]), 1);
  assert_eq((run("s65537"), s->a.i16[1]), 0);
  // type changes
  assert_eq((run("bs"), s->_type), INT16);
  assert_eq((run("bi"), s->_type), INT32);
  assert_eq((run("bf"), s->_type), FLOAT);
  assert_eq((run("ib"), s->_type), INT8);
  // random values
  assert_eq((run("s257"), s->a.i16[0]), 257);
  assert_eq((run("i65537"), s->a.i32), 65537);
  assert_eq((run("f16"), s->a.f32), 16);
  assert_eq((run("f16@"), s->a.f32), 0);
  assert_eq((run("f16@32"), s->a.f32), 32);
  assert_eq((run("f16@32"), s->b.f32), 16);
  // store 3/2
  assert_eq((run("f3@2@/"), s->a.f32), 3.0f/2.0f);
  end_section();

  begin_section("STORE");
  assert_eq((run("3!"), (*M)[0]), 3);
  end_section();

  // tape-register notation
  // M[n-1] [A|B> M[n]

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
  assert_eq((run(">4!<3!1"), s->a.i8[0]), 1);
  // [3|0> 3 4
  assert_eq((run(">4!<3!1;"), s->a.i8[0]), 3);
  // 0. [0|0>     - start
  // 1. ? [0|0>   - > right
  // 2. ? [4|0>   - 4 read const 4 into A
  // 3. ? [4|0> 4 - ! store
  // 4. [4|0> ? 4 - < left
  // 5. [3|0> ? 4 - 3 read const 3 into A
  // 6. [3|0> 3 4 - ! store
  // 7. [1|0> 3 4 - 1 read const 1 into A
  // 8. 3 [1|0> 4 - < right
  // 9. 3 [4|0> 4 - ; load
  assert_eq((run(">4!<3!1>;"), s->a.i8[0]), 4);
  end_section();

  begin_section("STRING");
  // register contains string length
  assert_eq((run("2!\"abc\""), s->a.i8[0]), 3);
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
  assert_eq((run("2!\"a\\\"c\">3!"), s->a.i8[0]), 3);
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
  assert_eq((run("2"), s->a.i8[0]), 2);
  assert_eq((run("2@"), s->b.i8[0]), 2);
  assert_eq((run("2@3"), s->a.i8[0]), 3);
  assert_eq((run("2@3"), s->b.i8[0]), 2);
  end_section();

  begin_section("COMP");
  assert_eq((run("0@1?<"), s->_ans), 0);
  assert_eq((run("1@1?<"), s->_ans), 0);
  assert_eq((run("2@1?<"), s->_ans), 1);
  assert_eq((run("0@1@?>"), s->_ans), 0);
  assert_eq((run("1@1@?>"), s->_ans), 0);
  assert_eq((run("2@1@?>"), s->a.i8[0]), 2);
  assert_eq((run("2@1@?>"), s->b.i8[0]), 1);
  assert_eq((run("2@1@?>"), s->_ans), 1);
  end_section();

  begin_section("OP");
  // 0. [0|0>     - start
  // 0. [12|0>    - 12 read const 12 into A
  // 0. [0|12>    - @  switch A, B
  // 0. [1|12>    - 1  read const 1 into A
  // 0. [13|12>   - +  A + B -> A
  assert_eq((run("12@1+"), s->a.i8[0]), 13);
  assert_eq((run("1@12-"), s->a.i8[0]), 11);
  assert_eq((run("1@12--"), s->a.i8[0]), 10);
  assert_eq((run("2@3*"), s->a.i8[0]), 6);
  end_section();

  begin_section("IF");
  // valid empty expressions
  assert_eq(run("()"), 0);
  assert_eq(run("(:)"), 0);
  // Ans is false by default
  assert_eq((run(""), s->_ans), 0);
  assert_eq(run("1(2)"), 0);
  assert_eq((run("1(2)"), s->a.i8[0]), 1);
  assert_eq((run("1(2:)"), s->a.i8[0]), 1);
  assert_eq((run("1(:2)"), s->a.i8[0]), 2);
  // false 
  assert_eq(run("?!(1:2)"), 0);
  assert_eq((run("?!(1:2)"), s->a.i8[0]), 2);
  assert_eq((run("(2:3)"), s->a.i8[0]), 3);
  // true
  assert_eq(run("?=(1:2)"), 0);
  assert_eq((run("?=(1:2)"), s->_ans), 1);
  assert_eq((run("?=(1:2)"), s->a.i8[0]), 1);
  // 0 > 1 ?
  assert_eq((run("0@1@?>(2:3)!"), s->_m[0]), 3);
  // 1 > 1 ?
  assert_eq((run("1@1@?>(2:3)!"), s->_m[0]), 3);
  // 2 > 1 ?
  assert_eq((run("2@1@?>(2:3)!"), s->_m[0]), 2);
  end_section();

  begin_section("Stack");
  assert_eq((run("b$3p4p"), (*M)[0]), 3);
  assert_eq((run("b$3p4p"), (*M)[1]), 4);
  assert_eq((run("b$3p4ph"), s->a.i16[0]), 2);
  assert_eq((run("b$3p4po"), s->a.i8[0]), 4);
  assert_eq((run("b$3p4poo"), s->a.i8[0]), 3);
  assert_eq((run("b$3p4pooh"), s->a.i16[0]), 0);
  assert_eq((run("b$1p2p3p4p5p 0@o+@o+@o+@o+@o+"), s->a.i8[0]), 15);
  assert_eq((run("b$1p2p3p4p5p 0?=[ @o+ !h??;]"), s->a.i8[0]), 15);
  end_section();

  begin_section("WHILE");
  assert_eq(run("[]"), 0);
  assert_eq(run("1[2]"), 0);
  assert_eq((run("1[2]"), s->a.i8[0]), 1);
  assert_eq(run("?=[x]"), 0);
  assert_eq(run("1[2x3]"), 0);
  assert_eq((run("1[2x3]"), s->a.i8[0]), 1);
  assert_eq(run("?=1[2x3]"), 0);
  assert_eq((run("?=1[2x3]"), s->a.i8[0]), 2);
  assert_eq(run("1?![x]"), 0);
  assert_eq((run("1?![x]"), s->a.i8[0]), 1);
  assert_eq(run("1@1?![-?!]"), 0);
  assert_eq((run("12@?![+?!]"), s->a.i8[0]), 12);
  assert_eq((run("12@?![+?!]"), s->b.i8[0]), 12);
  assert_eq((run("8!?![1@;-!?! c x ]"), s->a.i8[0]), 1);
  end_section();

  begin_section("Fibonacci");
  assert_eq((run("8!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->a.i8[0]), 21);
  assert_eq((run("9!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->a.i8[0]), 34);
  assert_eq((run("10!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->a.i8[0]), 55);
  assert_eq((run("i46!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->a.i32), 1836311903);
  end_section();

  begin_section("blockrun");
  char * code = "i46!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]";
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
    assert_eq(s->a.i32, 1836311903);
  }
  free(block);
  end_section();

  begin_section("IDS");
  assert_eq((run("ABC^1!>2!>3!ABC ;"), s->a.i8[0]), 1);
  assert_eq((run("N^8!>A^0!>B^1!?![1@N;-!?!B;@A;@!+B!]"), s->a.i8[0]), 21);
  assert_eq((run("N^>A^0!>B^1!N8!?![1@N;-!?!B;@A;@!+B!]"), s->a.i8[0]), 21);
  end_section();

  begin_section("Functions");
  assert_eq((run("X "), (*M)[0]), 123);
  assert_eq((run("Y "), (*M)[0]), 100);
  assert_eq((run("X Y "), (*M)[0]), 100);
  // undef
  assert_eq((run("X Y Z "), (*M)[0]), 111);
  end_section();

  begin_section("Functions2"); // TODO: fix space after call
  assert_eq((run("Z{1!}2!Z "), (*M)[0]), 1);
  assert_eq((run("Z{1!}2!>Z "), (*M)[0]), 2);
  assert_eq((run("Z{1!}2!>Z "), (*M)[1]), 1);
  // inner undef
  assert_eq((run("Z{1!K }2!>Z "), (*M)[1]), 111);
  assert_eq((run("Z{K{1!}K }2!>Z "), (*M)[1]), 1);
  assert_eq((run("FIB{;>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]}8!FIB2>;"), s->a.i32), 21);
  assert_eq((run("iFIB{i;>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]}46!FIB2>;"), s->a.i32), 1836311903);
  end_section();

  begin_section("Run");
  assert_eq((run("\"1!\"#"), (*M)[0]), 1);
  assert_eq((run("\"Z{1!K }2!>Z \"#"), (*M)[1]), 111);
  assert_eq((run("\"Z{1!K }2!\"#>Z "), (*M)[1]), 111);
  end_section();

  begin_section("Return");
  assert_eq((run("r1!"), (*M)[0]), 0);
  assert_eq((run("1!r2!"), (*M)[0]), 1);
  assert_eq((run("Z{11!rK }2!>Z "), (*M)[1]), 11);
  assert_eq((run("\"Z{11!rK }2!\"#>Z "), (*M)[1]), 11);
  end_section();

  end_tests();

  printf("state size: %ld\n", sizeof(State));

  return 0;
}
