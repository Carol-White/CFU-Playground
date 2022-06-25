/*
 * Copyright 2021 The rvv-Playground Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "functional_rvv_tests.h"

#include <stdio.h>
#include <riscv_vector.h>
#include <cstdlib>

#include "base.h"
#include "rvvLite.h"
#include "menu.h"
#include "riscv.h"

namespace {

void print_result(int op, uint32_t v0, uint32_t v1, uint32_t r);
void print_result_t(int op, uint32_t v0, uint32_t v1, uint32_t vd, uint32_t r);

void do_add_test(void) {
  vsetvl_e32m8(64);

  vuint32m8_t vA, vB, vC;

  uint32_t v0[32], v1[32], v2[32];

  for (int i = 0; i < 32; i++) {
    v0[i] = i;
    v1[i] = i + 1;
  }

  vA = vle32_v_u32m8(v0, 8);
  vB = vle32_v_u32m8(v1, 8);
  vC = vadd_vv_u32m8(vA, vB, 8);

  vse32_v_u32m8(v2, vC, 8);
  printf("Done test");
}

void do_add_test_macro(void) {
  rvv_opvv_hw(7, 98, "v8", "v19", "v0");
  rvv_opvv_hw(7, 98, "v24", "v8", "v24");
  printf("Done test");
}

void do_fixed_tests(void) {
  puts("RVV-Lite TEST for op0, 1 and 2:");
  puts("arg0        arg1        op0         op1         op2");
  for (uint32_t i = 0; i < 0x50505; i += 0x8103) {
    uint32_t j = i ^ 0xffff;
    uint32_t v0 = rvv_op(0, 0, i, j);
    uint32_t v1 = rvv_op(1, 0, i, j);
    uint32_t v2 = rvv_op(2, 0, i, j);
    printf("0x%08lx, 0x%08lx: 0x%08lx, 0x%08lx, 0x%08lx\n", i, j, v0, v1, v2);
  }
}

void do_compare_tests(void) {
  puts("RVV-Lite COMPARE TEST for op0, 1, and 2:");
  int count = 0;
  for (uint32_t i = 0; i < 0xff000000u; i += 0x710005u) {
    for (uint32_t j = 0; j < 0xff000000u; j += 0xb100233u) {
      uint32_t hw0 = rvv_op_hw(0, 0, i, j);
      uint32_t hw1 = rvv_op_hw(1, 0, i, j);
      uint32_t hw2 = rvv_op_hw(2, 0, i, j);
      uint32_t sw0 = rvv_op_sw(0, 0, i, j);
      uint32_t sw1 = rvv_op_sw(1, 0, i, j);
      uint32_t sw2 = rvv_op_sw(2, 0, i, j);
      if (hw0 != sw0 || hw1 != sw1 || hw2 != sw2) {
        puts(
            "arg0        arg1            fn0               fn1              "
            "fn2");
        printf(
            "0x%08lx, 0x%08lx: 0x%08lx:0x%08lx, 0x%08lx:0x%08lx, "
            "0x%08lx:0x%08lx <<=== "
            "MISMATCH!\n",
            i, j, hw0, sw0, hw1, sw1, hw2, sw2);
      }
      ++count;
      if ((count & 0xffff) == 0) printf("Ran %d comparisons....\n", count);
    }
  }
  printf("Ran %d comparisons.\n", count);
}

void print_result(int op, uint32_t v0, uint32_t v1, uint32_t r) {
  printf(
      "rvv_op%1d(%08lx, %08lx) = %08lx (hex), %ld (signed), %lu (unsigned)\n",
      op, v0, v1, r, r, r);
}

void print_result_t(int op, uint32_t v0, uint32_t v1, uint32_t vd, uint32_t r) {
  printf(
      "rvv_op%1d(%08lx, %08lx, %08lx) = %08lx (hex), %ld (signed), %lu (unsigned)\n",
      op, v0, v1, vd, r, r, r);
}

void do_interactive_tests(void) {
  puts("RVV-Lite Interactive Test:");

  uint32_t v0 = read_val("  First operand value  ");
  uint32_t v1 = read_val("  Second operand value ");
  print_result(0, v0, v1, rvv_op(0, 0, v0, v1));
  print_result(1, v0, v1, rvv_op(1, 0, v0, v1));
  print_result(2, v0, v1, rvv_op(2, 0, v0, v1));
  print_result(3, v0, v1, rvv_op(3, 0, v0, v1));
  print_result(4, v0, v1, rvv_op(4, 0, v0, v1));
  print_result(5, v0, v1, rvv_op(5, 0, v0, v1));
  print_result(6, v0, v1, rvv_op(6, 0, v0, v1));
  print_result(7, v0, v1, rvv_op(7, 0, v0, v1));
}

void do_opvv_tests(void) {
  puts("RVV-Lite OPVV Test:");

  // uint32_t v0 = read_val("  First vector source ");
  // uint32_t v1 = read_val("  Second vector source ");
  // uint32_t vd = read_val("  Vector dest ");

  // rvv_opvv_hw(0, 0, "v0", "v1", "v2");
  // r = rvv_opvv_hw(0, 0, 1, 2, 3);
  // r = rvv_opvv_hw(0, 0, 1, 2, 3);
  // r = rvv_opvv_hw(0, 0, 1, 2, 3);

  print_result_t(0, 2, 1, 0, rvv_opvv_hw(5, 4, "v2", "v1", "v0"));

  uint32_t v0 = read_val("  First operand value  ");
  uint32_t v1 = read_val("  Second operand value ");
  print_result(0, v0, v1, rvv_op(0, 0, v0, v1));
  // print_result_t(1, v0, v1, vd, rvv_opvv_hw(1, 0, v0, v1, vd));
  // print_result_t(2, v0, v1, vd, rvv_opvv_hw(2, 0, v0, v1, vd));
  // print_result_t(3, v0, v1, vd, rvv_opvv_hw(3, 0, v0, v1, vd));
  // print_result_t(4, v0, v1, vd, rvv_opvv_hw(4, 0, v0, v1, vd));
  // print_result_t(5, v0, v1, vd, rvv_opvv_hw(5, 0, v0, v1, vd));
  // print_result_t(6, v0, v1, vd, rvv_opvv_hw(6, 0, v0, v1, vd));
  // print_result_t(7, v0, v1, vd, rvv_opvv_hw(7, 0, v0, v1, vd));
}

void rip_my_youth () {
  asm(".word 0xc5347057;"); 
  asm(".word 0x02056c07;"); 
  asm(".word 0x0205e407;");
  asm(".word 0x5e03B057;");
  // asm(".word 0x03840c57;"); 
  // asm(".word 0x0207ec27;");
  print_result(0, 0, 0, 0);
}

struct Menu MENU = {
    "Tests for Functional rvvs",
    "functional",
    {
        MENU_ITEM('f', "Run fixed rvv tests", do_fixed_tests),
        MENU_ITEM('c', "Run hw/sw compare tests", do_compare_tests),
        MENU_ITEM('i', "Run interactive tests", do_interactive_tests),
        MENU_ITEM('v', "Run OPVV tests", do_opvv_tests),
        MENU_ITEM('a', "Run ADD test", do_add_test),
        MENU_ITEM('m', "Run ADD test (macro)", do_add_test_macro),
        MENU_ITEM('k', "Kill me", rip_my_youth),
        MENU_END,
    },
};

};  // anonymous namespace

extern "C" void do_functional_rvv_tests() { menu_run(&MENU); }
