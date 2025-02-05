/*
 * Copyright 2021 The CFU-Playground Authors
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

#include "base.h"

#include <console.h>
#include <generated/csr.h>
#include <hw/common.h>
#include <irq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uart.h>

#include "instruction_handler.h"
#include "riscv.h"

void isr(void) {
  __attribute__((unused)) unsigned int irqs;

  irqs = irq_pending() & irq_getmask();

  if (irqs & (1 << UART_INTERRUPT)) {
    uart_isr();
  }
}

void trap_handler(uint32_t* reg_base) {
  if (csr_read(mcause) == CAUSE_ILLEGAL_INSTRUCTION) {
    instruction_handler(reg_base);
  } else {
    isr();
  }
}

void init_runtime() {
  irq_setmask(0);
  irq_setie(1);
  uart_init();
}

uint32_t read_val(const char* prompt) {
  printf("%s > ", prompt);
  char buf[81];
  char c = readchar();
  int i = 0;
  while (i < 80 && c != '\r' && c != '\n') {
    buf[i++] = c;
    putchar(c);
    c = readchar();
  }
  buf[i] = '\0';
  putchar('\n');
  return strtol(buf, NULL, 0);
}

void print_float(const char *name, float x)
{
  char m = ' ';
  if (x < 0.0f) {
      m = '-';
      x = -x;
  }
  int i_i = (int)x;
  float r = x - (float)(i_i);
  printf("%s: %c%d.%03d\n", name, m, i_i, (int)(r * 1000.0f));
}