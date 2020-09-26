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
  s->block = (uint8_t*) src;
  int8_t result = blockrun(s);
  M = &(s->mem_begin);
  return result;
}

void api(uint8_t pre, uint8_t op, State * s){
  switch (pre) {
    case 0:
      switch (op) {
        case IN:
          scanf("%d", &(s->A.i32));
          break;
        case OUT:
          printf("%d\n", s->A.i32);
          break;
      }
      break;
    default:
      //err TODO
      break;
  }
}

void clean(){
  free(s->mem_begin); 
  free(s);
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
  assert_eq((run(""), s->Type), INT8);
  assert_eq((run(""), s->A.f32), 0);
  assert_eq((run(""), s->B.f32), 0);
  // random assertions
  assert_eq((run("3"), s->A.i8[0]), 3);
  assert_eq((run("1"), s->A.i8[0]), 1);
  assert_eq((run("10"), s->A.i8[0]), 10);
  assert_eq((run("20"), s->A.i8[0]), 20);
  assert_eq((run("33"), s->A.i8[0]), 33);
  assert_eq((run("128"), s->A.i8[0]), 128);
  assert_eq((run("255"), s->A.i8[0]), 255);
  assert_eq((run("256"), s->A.i8[0]), 0);
  // integer overflow
  assert_eq((run("257"), s->A.i8[0]), 1);
  assert_eq((run("257"), s->A.i8[1]), 0);
  assert_eq((run("s65537"), s->A.i16[0]), 1);
  assert_eq((run("s65537"), s->A.i16[1]), 0);
  // type changes
  assert_eq((run("bs"), s->Type), INT16);
  assert_eq((run("bi"), s->Type), INT32);
  assert_eq((run("bf"), s->Type), FLOAT);
  assert_eq((run("ib"), s->Type), INT8);
  // random values
  assert_eq((run("s257"), s->A.i16[0]), 257);
  assert_eq((run("i65537"), s->A.i32), 65537);
  assert_eq((run("f16"), s->A.f32), 16);
  assert_eq((run("f16@"), s->A.f32), 0);
  assert_eq((run("f16@32"), s->A.f32), 32);
  assert_eq((run("f16@32"), s->B.f32), 16);
  // store 3/2
  assert_eq((run("f3@2@/"), s->A.f32), 3.0f/2.0f);
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
  assert_eq((run(">4!<3!1"), s->A.i8[0]), 1);
  // [3|0> 3 4
  assert_eq((run(">4!<3!1;"), s->A.i8[0]), 3);
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
  assert_eq((run(">4!<3!1>;"), s->A.i8[0]), 4);
  end_section();

  begin_section("SWITCH");
  assert_eq((run("2"), s->A.i8[0]), 2);
  assert_eq((run("2@"), s->B.i8[0]), 2);
  assert_eq((run("2@3"), s->A.i8[0]), 3);
  assert_eq((run("2@3"), s->B.i8[0]), 2);
  end_section();

  begin_section("COMP");
  assert_eq((run("0@1?<"), s->Ans), 0);
  assert_eq((run("1@1?<"), s->Ans), 0);
  assert_eq((run("2@1?<"), s->Ans), 1);
  assert_eq((run("0@1@?>"), s->Ans), 0);
  assert_eq((run("1@1@?>"), s->Ans), 0);
  assert_eq((run("2@1@?>"), s->A.i8[0]), 2);
  assert_eq((run("2@1@?>"), s->B.i8[0]), 1);
  assert_eq((run("2@1@?>"), s->Ans), 1);
  end_section();

  begin_section("OP");
  // 0. [0|0>     - start
  // 0. [12|0>    - 12 read const 12 into A
  // 0. [0|12>    - @  switch A, B
  // 0. [1|12>    - 1  read const 1 into A
  // 0. [13|12>   - +  A + B -> A
  assert_eq((run("12@1+"), s->A.i8[0]), 13);
  assert_eq((run("1@12-"), s->A.i8[0]), 11);
  assert_eq((run("1@12--"), s->A.i8[0]), 10);
  assert_eq((run("2@3*"), s->A.i8[0]), 6);
  end_section();

  begin_section("IF");
  // valid empty expressions
  assert_eq(run("()"), 0);
  assert_eq(run("(:)"), 0);
  // Ans is false by default
  assert_eq((run(""), s->Ans), 0);
  assert_eq(run("1(2)"), 0);
  assert_eq((run("1(2)"), s->A.i8[0]), 1);
  assert_eq((run("1(2:)"), s->A.i8[0]), 1);
  assert_eq((run("1(:2)"), s->A.i8[0]), 2);
  // false 
  assert_eq(run("?!(1:2)"), 0);
  assert_eq((run("?!(1:2)"), s->A.i8[0]), 2);
  assert_eq((run("(2:3)"), s->A.i8[0]), 3);
  // true
  assert_eq(run("?=(1:2)"), 0);
  assert_eq((run("?=(1:2)"), s->Ans), 1);
  assert_eq((run("?=(1:2)"), s->A.i8[0]), 1);
  // 0 > 1 ?
  assert_eq((run("0@1@?>(2:3)!"), s->Mem[0]), 3);
  // 1 > 1 ?
  assert_eq((run("1@1@?>(2:3)!"), s->Mem[0]), 3);
  // 2 > 1 ?
  assert_eq((run("2@1@?>(2:3)!"), s->Mem[0]), 2);
  end_section();

  begin_section("Stack");
  assert_eq((run("b$3p4p"), (*M)[0]), 3);
  assert_eq((run("b$3p4p"), (*M)[1]), 4);
  assert_eq((run("b$3p4ph"), s->A.i16[0]), 2);
  assert_eq((run("b$3p4po"), s->A.i8[0]), 4);
  assert_eq((run("b$3p4poo"), s->A.i8[0]), 3);
  assert_eq((run("b$3p4pooh"), s->A.i16[0]), 0);
  assert_eq((run("b$1p2p3p4p5p 0@o+@o+@o+@o+@o+"), s->A.i8[0]), 15);
  assert_eq((run("b$1p2p3p4p5p 0?=[ @o+ !h??;]"), s->A.i8[0]), 15);
  end_section();

  begin_section("WHILE");
  assert_eq(run("[]"), 0);
  assert_eq(run("1[2]"), 0);
  assert_eq((run("1[2]"), s->A.i8[0]), 1);
  assert_eq(run("?=[x]"), 0);
  assert_eq(run("1[2x3]"), 0);
  assert_eq((run("1[2x3]"), s->A.i8[0]), 1);
  assert_eq(run("?=1[2x3]"), 0);
  assert_eq((run("?=1[2x3]"), s->A.i8[0]), 2);
  assert_eq(run("1?![x]"), 0);
  assert_eq((run("1?![x]"), s->A.i8[0]), 1);
  assert_eq(run("1@1?![-?!]"), 0);
  assert_eq((run("12@?![+?!]"), s->A.i8[0]), 12);
  assert_eq((run("12@?![+?!]"), s->B.i8[0]), 12);
  assert_eq((run("8!?![1@;-!?! c x ]"), s->A.i8[0]), 1);
  end_section();

  begin_section("Fibonacci");
  assert_eq((run("8!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 21);
  assert_eq((run("9!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 34);
  assert_eq((run("10!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 55);
  assert_eq((run("i46!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i32), 1836311903);
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
      s->block = (uint8_t*) block;
      direction = blockrun(s);
      // printf("%s %d %d %d\n", block, block_size, direction, index);

      if (direction == -1){
        index--;
      } else if (direction == 0){
        index++;
      } else {
        //error
        printf("%d\n", direction);
        break;
      }
    }
    assert_eq(s->A.i32, 1836311903);
  }
  free(block);
  end_section();

  end_tests();

  return 0;
}
