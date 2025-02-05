/*
 * Copyright 2021 The rvvLite-Playground Authors
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

#include <stdint.h>

#include "software_rvvLite.h"

//
// Create your own version of this function by copying this file to your
// project's src directory, then modifying it.
//
// In this function, place C code to emulate your rvvLite. You can switch between
// hardware and emulated rvvLite by setting the rvvLite_SOFTWARE_DEFINED DEFINE in
// the Makefile.
uint32_t software_rvvLite(int funct3, int funct7, uint32_t rs1, uint32_t rs2) {
  (void)rs1;
  (void)rs2;
  return 0;
}
