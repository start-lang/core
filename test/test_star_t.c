#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microcuts.h>
#include <star_t.h>

//suppress pointer signedness warning during compilation
#define run(A) runall((uint8_t*) A)

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

int main(void){
  uint8_t ** M = &mem_begin;

  begin_section("NOP");
  assert_eq(run(" "), 0);
  assert_eq(run("   "), 0);
  // assert_eq(run(" ` "), -1);
  end_section();

  begin_section("DATA TYPES");
  assert_eq((run("3"), s->Type), INT8);
  assert_eq((run("3"), s->A.i8[0]), 3);
  assert_eq((run("1"), s->A.i8[0]), 1);
  assert_eq((run("10"), s->A.i8[0]), 10);
  assert_eq((run("20"), s->A.i8[0]), 20);
  assert_eq((run("33"), s->A.i8[0]), 33);
  assert_eq((run("128"), s->A.i8[0]), 128);
  assert_eq((run("255"), s->A.i8[0]), 255);
  assert_eq((run("256"), s->A.i8[0]), 0);
  assert_eq((run("257"), s->A.i8[0]), 1);
  assert_eq((run("257"), s->A.i8[1]), 0);
  assert_eq((run("b"), s->Type), INT8);
  assert_eq((run("b"), s->A.i8[0]), 0);
  assert_eq((run("b3"), s->Type), INT8);
  assert_eq((run("b3"), s->A.i8[0]), 3);
  assert_eq((run("b33"), s->A.i8[0]), 33);
  assert_eq((run("b128"), s->A.i8[0]), 128);
  assert_eq((run("b255"), s->A.i8[0]), 255);
  assert_eq((run("b256"), s->A.i8[0]), 0);
  assert_eq((run("b257"), s->A.i8[0]), 1);
  assert_eq((run("b257"), s->A.i8[1]), 0);

  assert_eq((run("s3"), s->Type), INT16);
  assert_eq((run("i3"), s->Type), INT32);
  assert_eq((run("f3"), s->Type), FLOAT);
  // assert_eq(run("f3"), 0);
  end_section();

  begin_section("STORE");
  assert_eq((run("3!"), (*M)[0]), 3);
  end_section();

  begin_section("Move/Load");
  assert_eq((run("3!>4!"), (*M)[0]), 3);
  assert_eq((run("3!>4!"), (*M)[1]), 4);
  assert_eq((run(">4!<3!"), (*M)[0]), 3);
  assert_eq((run(">4!<3!"), (*M)[1]), 4);
  assert_eq((run(">4!<3!1"), s->A.i8[0]), 1);
  assert_eq((run(">4!<3!1;"), s->A.i8[0]), 3);
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
  assert_eq((run("12@1+"), s->A.i8[0]), 13);
  assert_eq((run("1@12-"), s->A.i8[0]), 11);
  assert_eq((run("1@12--"), s->A.i8[0]), 10);
  assert_eq((run("2@3*"), s->A.i8[0]), 6);
  end_section();

  begin_section("IF");
  assert_eq(run("()"), 0);
  assert_eq(run("(:)"), 0);
  assert_eq(run("1(2)"), 0);
  assert_eq((run("1(2)"), s->A.i8[0]), 1);
  assert_eq((run("1(2:)"), s->A.i8[0]), 1);
  assert_eq((run("1(:2)"), s->A.i8[0]), 2);
  assert_eq((run("?!(1:2)"), s->A.i8[0]), 2);
  assert_eq(run("?=(1:2)"), 0);
  assert_eq(run("?!(1:2)"), 0);
  assert_eq((run("(2:3)"), s->A.i8[0]), 3);
  assert_eq(run("?=(1:2)"), 0);
  assert_eq((run("?=(1:2)"), s->Ans), 1);
  assert_eq((run("?=(1:2)"), s->A.i8[0]), 1);
  assert_eq((run("0@1@?>(2:3)!"), s->Mem[0]), 3);
  assert_eq((run("1@1@?>(2:3)!"), s->Mem[0]), 3);
  assert_eq((run("2@1@?>(2:3)!"), s->Mem[0]), 2);
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

  begin_section("Fibonacci");
  assert_eq((run("8!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 21);
  assert_eq((run("9!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 34);
  assert_eq((run("10!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i8[0]), 55);
  assert_eq((run("i46!>i0!>i1!?![2<i1@;-!?!2> ;@<;@!+>!]"), s->A.i32), 1836311903);
  assert_eq((run("ti46!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]"), s->A.i32), 1836311903);
  end_section();

  begin_section("blockrun");
  char * code = "ti46!>0!>1!?![2<1@;-!?!2> ;@<;@!+>!]";
  for (int max_block_size = 1; max_block_size <= strlen(code); max_block_size++){
    int direction = 0;
    int index = 0;
    begin();
    char * block = realloc(block, (max_block_size+1)*sizeof(char));
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
      direction = blockrun((uint8_t*)block, block_size);
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
  end_section();

  end_tests();

  return 0;
}
