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

void do_add_test(void) {
  vsetvl_e16m2(64);

  vuint16m2_t vA, vB, vC;

  uint16_t v0[8], v1[8], v2[8], v2_ex[8];

  for (int i = 0; i < 8; i++) {
    v0[i] = i;
    v1[i] = i + 1;

    v2_ex[i] = v0[i] + v1[i];
  }

  vA = vle16_v_u16m2(v0, 8);
  vB = vle16_v_u16m2(v1, 8);
  // vA = vmv_v_x_u32m4(15, 8);
  // vB = vid_v_u16m2(8);

  vC = vadd_vv_u16m2(vA, vB, 8);

  vse16_v_u16m2(v2, vC, 8);

  // wait for memory to return
  for (int i = 0; i < 150; i++) {
    printf("syncing memory...\n");
  }

  for (int i = 0; i < 8; i++) {
    printf("Got %d, expected %d\n", v2[i], v2_ex[i]);
  }

  printf("Finished ADD test\n");
}

// FIXME why does this hang???
void do_mask_logic_test(void) {
  vsetvl_e32m4(64);

  vint32m4_t vA, vB, vC;
  vbool8_t vm0, vm1, vm2;

  int32_t v0[8], v1[8], v2[8];

  for (int i = 0; i < 8; i++) {
    v0[i] = i;
    v1[i] = i + 1;
  }

  // vA = vle16_v_u16m2(v0, 8);
  // vB = vle16_v_u16m2(v1, 8);
  vA = vmv_v_x_i32m4(2, 8);
  vB = vid_v_i32m4(8);

  vm0 = vmsgt_vv_i32m4_b8(vB, vA, 8);
  vm1 = vmseq_vv_i32m4_b8(vA, vB, 8);
  vm2 = vmxor_mm_b8(vm0, vm1, 8);


  vC = vadd_vv_i32m4_m(vm2, vC, vA, vB, 8);

  vse32_v_i32m4(v2, vC, 8);

  vA = vle32_v_i32m4(v2, 8);
  vA = vadd_vv_i32m4(vA, vB, 8);

  vse32_v_i32m4(v1, vA, 8);

  printf("Finished MASK test\n");
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

  print_result_t(0, 2, 1, 3, rvv_opvv_hw(5, 4, "v2", "v1", "v3"));

  // uint32_t v0 = read_val("  First operand value  ");
  // uint32_t v1 = read_val("  Second operand value ");
  // print_result(0, v0, v1, rvv_op(0, 0, v0, v1));
  // print_result_t(1, v0, v1, vd, rvv_opvv_hw(1, 0, v0, v1, vd));
  // print_result_t(2, v0, v1, vd, rvv_opvv_hw(2, 0, v0, v1, vd));
  // print_result_t(3, v0, v1, vd, rvv_opvv_hw(3, 0, v0, v1, vd));
  // print_result_t(4, v0, v1, vd, rvv_opvv_hw(4, 0, v0, v1, vd));
  // print_result_t(5, v0, v1, vd, rvv_opvv_hw(5, 0, v0, v1, vd));
  // print_result_t(6, v0, v1, vd, rvv_opvv_hw(6, 0, v0, v1, vd));
  // print_result_t(7, v0, v1, vd, rvv_opvv_hw(7, 0, v0, v1, vd));
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
        MENU_ITEM('m', "Run MASK test", do_mask_logic_test),
        MENU_END,
    },
};

};  // anonymous namespace

extern "C" void do_functional_rvv_tests() { menu_run(&MENU); }
