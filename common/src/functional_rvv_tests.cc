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
// #include "rvvLite.h"
#include "menu.h"
// #include "riscv.h"

namespace {

void do_basic_ld_st_test(void) {
  int num_elems = 64;

  vsetvl_e32m4(num_elems);

  int32_t v0[num_elems];
  int32_t v1[num_elems];

  vint32m4_t vA;

  for (int i = 0; i < num_elems; i++){
    v0[i] = (int32_t)i;
    v1[i] = 1;
  }

  vA = vle32_v_i32m4(v0, num_elems);

  vse32_v_i32m4(v1, vA, num_elems);

  for (int i = 0; i < num_elems; i++) {
    printf("v1: %0x. %s. Got %d, expected %d\n", &(v1[i]), (v1[i] == v0[i] ? "PASS" : "FAIL"), v1[i], v0[i]);
  }

  printf("Done basic ld/st test");
}

void do_add_test(void) {
  int num_elems = 64;

  vsetvl_e16m2(num_elems);
  vuint16m2_t vA, vB, vC;

  uint16_t v0[num_elems];
  uint16_t v1[num_elems];
  uint16_t v2[num_elems];
  uint16_t v2_ex[num_elems];

  for (int i = 0; i < num_elems; i++) {
    v0[i] = (int32_t)i;
    v1[i] = (int32_t)(i + 1);
    v2[i] = 1;

    v2_ex[i] = v0[i] + v1[i];
  }

  vA = vle16_v_u16m2(v0, num_elems);
  vB = vle16_v_u16m2(v1, num_elems);

  vC = vadd_vv_u16m2(vA, vB, num_elems);

  vse16_v_u16m2(v2, vC, num_elems);

  int err_count = 0;
  for (int i = 0; i < num_elems; i++) {
    printf("v2: %0x. %s. Got %d, expected %d\n", &(v2[i]), (v2[i] == v2_ex[i] ? "PASS" : "FAIL"), v2[i], v2_ex[i]);
    
    if (v2[i] != v2_ex[i]) err_count++;
  }

  printf("Finished ADD test with %d errors.\n", err_count);
}

void do_mask_logic_test(void) {
  int num_elems = 2048;
  
  vsetvl_e32m4(num_elems);

  vint32m4_t vA, vB, vC;
  vbool8_t vm0, vm1, vm2;

  int32_t v0[num_elems];
  int32_t v1[num_elems];
  int32_t v2[num_elems];
  int32_t v3[num_elems];
  int32_t v2_ex[num_elems];
  int32_t v3_ex[num_elems];

  // printf("v2 addr: %0x\n", v2);

  for (int i = 0; i < num_elems; i++) {
    v0[i] = 2;
    v1[i] = (int32_t)i;
    v2[i] = 1;
    v3[i] = 1;

    if (v1[i] <= v0[i])
      v2_ex[i] = v0[i] + v1[i];
    else
      v2_ex[i] = 0;

    v3_ex[i] = v1[i] + v2_ex[i];
  }

  // vA = vle16_v_u16m2(v0, 8);
  // vB = vle16_v_u16m2(v1, 8);
  vA = vmv_v_x_i32m4(2, num_elems);
  vB = vid_v_i32m4(num_elems);

  vm0 = vmsgt_vv_i32m4_b8(vB, vA, num_elems);
  vm1 = vmseq_vv_i32m4_b8(vA, vB, num_elems);
  vm2 = vmxor_mm_b8(vm0, vm1, num_elems);

  vC = vadd_vv_i32m4_m(vm2, vC, vA, vB, num_elems);

  vse32_v_i32m4(v2, vC, num_elems);

  vC = vle32_v_i32m4(v2, num_elems);
  vA = vadd_vv_i32m4(vC, vB, num_elems);

  vse32_v_i32m4(v3, vA, num_elems);

  int err_count = 0;

  for (int i = 0; i < num_elems; i++) {
    if (v2[i] != v2_ex[i]){
      err_count++;
      printf("v2: %0x. %s. Got %d, expected %d\n", &(v2[i]), (v2[i] == v2_ex[i] ? "PASS" : "FAIL"), v2[i], v2_ex[i]);
    }
  }

  for (int i = 0; i < num_elems; i++) {
    if (v3[i] != v3_ex[i]){
      err_count++;
      printf("v3: %0x. %s. Got %d, expected %d\n", &(v3[i]), (v3[i] == v3_ex[i] ? "PASS" : "FAIL"), v3[i], v3_ex[i]);
    }
  }

  printf("Finished MASK test with %d errors\n", err_count);
}

struct Menu MENU = {
    "Tests for Functional rvvs",
    "functional",
    {
        MENU_ITEM('a', "Run ADD test", do_add_test),
        MENU_ITEM('m', "Run MASK test", do_mask_logic_test),
        MENU_ITEM('l', "Run LOAD/STORE test", do_basic_ld_st_test),
        MENU_END,
    },
};

};  // anonymous namespace

extern "C" void do_functional_rvv_tests() { menu_run(&MENU); }
