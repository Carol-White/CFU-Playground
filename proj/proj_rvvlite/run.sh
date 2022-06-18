#!/bin/bash

make TARGET=xilinx_zcu104 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite" clean
make TARGET=xilinx_zcu104 USE_VIVADO=1 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite" prog
make TTY=/dev/ttyUSB3 TARGET=xilinx_zcu104 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite" load
