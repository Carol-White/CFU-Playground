#!/bin/bash

make TARGET=xilinx_zcu104 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite" clean
make TARGET=xilinx_zcu104 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite --sys-clk-freq=250000000" prog
# make TTY=/dev/ttyUSB3 TARGET=xilinx_zcu104 EXTRA_LITEX_ARGS="--cpu-variant=rvvLite" load
