#
# This file is part of LiteX.
#
# Copyright (c) 2018 Dolu1990 <charles.papon.90@gmail.com>
# Copyright (c) 2018-2019 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2018-2019 Sean Cross <sean@xobs.io>
# Copyright (c) 2019 Tim 'mithro' Ansell <me@mith.ro>
# Copyright (c) 2019 David Shah <dave@ds0.me>
# Copyright (c) 2019 Antmicro <www.antmicro.com>
# Copyright (c) 2019 Kurt Kiefer <kekiefer@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

import os

from migen import *

from litex import get_data_mod
from litex.soc.interconnect import wishbone, axi
from litex.soc.interconnect.csr import *
from litex.soc.cores.cpu import CPU, CPU_GCC_TRIPLE_RISCV32

# Variants -----------------------------------------------------------------------------------------

CPU_VARIANTS = {
    "minimal":            "VexRiscv_Min",
    "minimal+debug":      "VexRiscv_MinDebug",
    "minimal+debug+hwbp": "VexRiscv_MinDebugHwBP",
    "lite":               "VexRiscv_Lite",
    "lite+debug":         "VexRiscv_LiteDebug",
    "lite+debug+hwbp":    "VexRiscv_LiteDebugHwBP",
    "standard":           "VexRiscv",
    "standard+debug":     "VexRiscv_Debug",
    "imac":               "VexRiscv_IMAC",
    "imac+debug":         "VexRiscv_IMACDebug",
    "full":               "VexRiscv_Full",
    "full+cfu":           "VexRiscv_FullCfu",
    "full+debug":         "VexRiscv_FullDebug",
    "full+cfu+debug":     "VexRiscv_FullCfuDebug",
    "linux":              "VexRiscv_Linux",
    "linux+debug":        "VexRiscv_LinuxDebug",
    "linux+no-dsp":       "VexRiscv_LinuxNoDspFmax",
    "secure":             "VexRiscv_Secure",
    "secure+debug":       "VexRiscv_SecureDebug",
    "rvvLite":            "VexRiscv_rvvLite",
}

# GCC Flags ----------------------------------------------------------------------------------------

GCC_FLAGS = {
    #                               /-------- Base ISA
    #                               |/------- Hardware Multiply + Divide
    #                               ||/----- Atomics
    #                               |||/---- Compressed ISA
    #                               ||||/--- Single-Precision Floating-Point
    #                               |||||/-- Double-Precision Floating-Point
    #                               imacfd
    "minimal":          "-march=rv32i      -mabi=ilp32",
    "minimal+debug":    "-march=rv32i      -mabi=ilp32",
    "lite":             "-march=rv32im     -mabi=ilp32",
    "lite+debug":       "-march=rv32im     -mabi=ilp32",
    "standard":         "-march=rv32im     -mabi=ilp32",
    "standard+debug":   "-march=rv32im     -mabi=ilp32",
    "imac":             "-march=rv32imac   -mabi=ilp32",
    "imac+debug":       "-march=rv32imac   -mabi=ilp32",
    "full":             "-march=rv32im     -mabi=ilp32",
    "full+cfu":         "-march=rv32im     -mabi=ilp32",
    "full+debug":       "-march=rv32im     -mabi=ilp32",
    "full+cfu+debug":   "-march=rv32im     -mabi=ilp32",
    "linux":            "-march=rv32ima    -mabi=ilp32",
    "linux+debug":      "-march=rv32ima    -mabi=ilp32",
    "linux+no-dsp":     "-march=rv32ima    -mabi=ilp32",
    "secure":           "-march=rv32ima    -mabi=ilp32",
    "secure+debug":     "-march=rv32ima    -mabi=ilp32",
    "rvvLite":          "-march=rv32imv    -mabi=ilp32",
}

# VexRiscv Timer -----------------------------------------------------------------------------------

class VexRiscvTimer(Module, AutoCSR):
    def __init__(self):
        self._latch    = CSR()
        self._time     = CSRStatus(64)
        self._time_cmp = CSRStorage(64, reset=2**64-1)
        self.interrupt = Signal()

        # # #

        time = Signal(64)
        self.sync += time.eq(time + 1)
        self.sync += If(self._latch.re, self._time.status.eq(time))

        time_cmp = Signal(64, reset=2**64-1)
        self.sync += If(self._latch.re, time_cmp.eq(self._time_cmp.storage))

        self.comb += self.interrupt.eq(time >= time_cmp)

# VexRiscv -----------------------------------------------------------------------------------------

class VexRiscv(CPU, AutoCSR):
    family               = "riscv"
    name                 = "vexriscv"
    human_name           = "VexRiscv"
    variants             = CPU_VARIANTS
    data_width           = 32
    endianness           = "little"
    gcc_triple           = CPU_GCC_TRIPLE_RISCV32
    linker_output_format = "elf32-littleriscv"
    nop                  = "nop"
    io_regions           = {0x80000000: 0x80000000} # Origin, Length

    # Memory Mapping.
    @property
    def mem_map(self):
        return {
            "rom":            0x00000000,
            "sram":           0x10000000,
            "main_ram":       0x40000000,
            "csr":            0xf0000000,
            "vexriscv_debug": 0xf00f0000,
        }

    # GCC Flags.
    @property
    def gcc_flags(self):
        flags = GCC_FLAGS[self.variant]
        flags += " -D__vexriscv__"
        return flags

    def __init__(self, platform, variant="standard", with_timer=False):
        self.platform         = platform
        self.variant          = variant
        self.human_name       = CPU_VARIANTS.get(variant, "VexRiscv")
        self.external_variant = None
        self.reset            = Signal()
        self.interrupt        = Signal(32)
        self.ibus             = ibus = wishbone.Interface()
        self.dbus             = dbus = wishbone.Interface()
        self.periph_buses     = [ibus, dbus] # Peripheral buses (Connected to main SoC's bus).
        self.memory_buses     = []           # Memory buses (Connected directly to LiteDRAM).

        # # #

        # CPU Instance.
        self.cpu_params = dict(
            i_clk                    = ClockSignal("sys"),
            i_reset                  = ResetSignal("sys") | self.reset,

            i_externalInterruptArray = self.interrupt,
            i_timerInterrupt         = 0,
            i_softwareInterrupt      = 0,

            o_iBusWishbone_ADR      = ibus.adr,
            o_iBusWishbone_DAT_MOSI = ibus.dat_w,
            o_iBusWishbone_SEL      = ibus.sel,
            o_iBusWishbone_CYC      = ibus.cyc,
            o_iBusWishbone_STB      = ibus.stb,
            o_iBusWishbone_WE       = ibus.we,
            o_iBusWishbone_CTI      = ibus.cti,
            o_iBusWishbone_BTE      = ibus.bte,
            i_iBusWishbone_DAT_MISO = ibus.dat_r,
            i_iBusWishbone_ACK      = ibus.ack,
            i_iBusWishbone_ERR      = ibus.err,

            o_dBusWishbone_ADR      = dbus.adr,
            o_dBusWishbone_DAT_MOSI = dbus.dat_w,
            o_dBusWishbone_SEL      = dbus.sel,
            o_dBusWishbone_CYC      = dbus.cyc,
            o_dBusWishbone_STB      = dbus.stb,
            o_dBusWishbone_WE       = dbus.we,
            o_dBusWishbone_CTI      = dbus.cti,
            o_dBusWishbone_BTE      = dbus.bte,
            i_dBusWishbone_DAT_MISO = dbus.dat_r,
            i_dBusWishbone_ACK      = dbus.ack,
            i_dBusWishbone_ERR      = dbus.err
        )

        # Add Timer (Optional).
        if with_timer:
            self.add_timer()

        # Add Debug (Optional).
        if "debug" in variant:
            self.add_debug()

    def set_reset_address(self, reset_address):
        self.reset_address = reset_address
        self.cpu_params.update(i_externalResetVector=Signal(32, reset=reset_address))

    def add_timer(self):
        self.submodules.timer = VexRiscvTimer()
        self.cpu_params.update(i_timerInterrupt=self.timer.interrupt)

    def add_debug(self):
        debug_reset = Signal()

        ibus_err = Signal()
        dbus_err = Signal()

        self.i_cmd_valid           = Signal()
        self.i_cmd_payload_wr      = Signal()
        self.i_cmd_payload_address = Signal(8)
        self.i_cmd_payload_data    = Signal(32)
        self.o_cmd_ready           = Signal()
        self.o_rsp_data            = Signal(32)
        self.o_resetOut            = Signal()

        reset_debug_logic = Signal()

        self.transfer_complete     = Signal()
        self.transfer_in_progress  = Signal()
        self.transfer_wait_for_ack = Signal()

        self.debug_bus = wishbone.Interface()

        self.sync += self.debug_bus.dat_r.eq(self.o_rsp_data)
        self.sync += debug_reset.eq(reset_debug_logic | ResetSignal())

        self.sync += [
            # CYC is held high for the duration of the transfer.
            # STB is kept high when the transfer finishes (write)
            # or the master is waiting for data (read), and stays
            # there until ACK, ERR, or RTY are asserted.
            If((self.debug_bus.stb & self.debug_bus.cyc)
            & (~self.transfer_in_progress)
            & (~self.transfer_complete)
            & (~self.transfer_wait_for_ack),
                self.i_cmd_payload_data.eq(self.debug_bus.dat_w),
                self.i_cmd_payload_address.eq((self.debug_bus.adr[0:6] << 2) | 0),
                self.i_cmd_payload_wr.eq(self.debug_bus.we),
                self.i_cmd_valid.eq(1),
                self.transfer_in_progress.eq(1),
                self.transfer_complete.eq(0),
                self.debug_bus.ack.eq(0)
            ).Elif(self.transfer_in_progress,
                If(self.o_cmd_ready,
                    self.i_cmd_valid.eq(0),
                    self.i_cmd_payload_wr.eq(0),
                    self.transfer_complete.eq(1),
                    self.transfer_in_progress.eq(0)
                )
            ).Elif(self.transfer_complete,
                self.transfer_complete.eq(0),
                self.debug_bus.ack.eq(1),
                self.transfer_wait_for_ack.eq(1)
            ).Elif(self.transfer_wait_for_ack & ~(self.debug_bus.stb & self.debug_bus.cyc),
                self.transfer_wait_for_ack.eq(0),
                self.debug_bus.ack.eq(0)
            ),
            # Force a Wishbone error if transferring during a reset sequence.
            # Because o_resetOut is multiple cycles and i.stb/d.stb should
            # deassert one cycle after i_err/i_ack/d_err/d_ack are asserted,
            # this will give i_err and o_err enough time to be reset to 0
            # once the reset cycle finishes.
            If(self.o_resetOut,
                If(self.ibus.cyc & self.ibus.stb, ibus_err.eq(1)).Else(ibus_err.eq(0)),
                If(self.dbus.cyc & self.dbus.stb, dbus_err.eq(1)).Else(dbus_err.eq(0)),
                reset_debug_logic.eq(1))
            .Else(
                reset_debug_logic.eq(0)
            )
        ]

        self.cpu_params.update(
            i_reset = ResetSignal() | self.reset | debug_reset,
            i_iBusWishbone_ERR              = self.ibus.err | ibus_err,
            i_dBusWishbone_ERR              = self.dbus.err | dbus_err,
            i_debugReset                    = ResetSignal(),
            i_debug_bus_cmd_valid           = self.i_cmd_valid,
            i_debug_bus_cmd_payload_wr      = self.i_cmd_payload_wr,
            i_debug_bus_cmd_payload_address = self.i_cmd_payload_address,
            i_debug_bus_cmd_payload_data    = self.i_cmd_payload_data,
            o_debug_bus_cmd_ready           = self.o_cmd_ready,
            o_debug_bus_rsp_data            = self.o_rsp_data,
            o_debug_resetOut                = self.o_resetOut
        )

    def add_cfu(self, cfu_filename):
        # Check CFU presence.
        if not os.path.exists(cfu_filename):
            raise OSError(f"Unable to find VexRiscv CFU plugin {cfu_filename}.")

        # CFU:CPU Bus Layout.
        cfu_bus_layout = [
            ("cmd", [
                ("valid", 1),
                ("ready", 1),
                ("payload", [
                    ("function_id", 10),
                    ("inputs_0", 32),
                    ("inputs_1", 32),
                ]),
            ]),
            ("rsp", [
                ("valid", 1),
                ("ready", 1),
                ("payload", [
                    ("outputs_0", 32),
                ]),
            ]),
        ]

        # The CFU:CPU Bus.
        self.cfu_bus = cfu_bus = Record(cfu_bus_layout)

        # Connect CFU to the CFU:CPU bus.
        self.cfu_params = dict(
            i_cmd_valid                = cfu_bus.cmd.valid,
            o_cmd_ready                = cfu_bus.cmd.ready,
            i_cmd_payload_function_id  = cfu_bus.cmd.payload.function_id,
            i_cmd_payload_inputs_0     = cfu_bus.cmd.payload.inputs_0,
            i_cmd_payload_inputs_1     = cfu_bus.cmd.payload.inputs_1,
            o_rsp_valid                = cfu_bus.rsp.valid,
            i_rsp_ready                = cfu_bus.rsp.ready,
            o_rsp_payload_outputs_0    = cfu_bus.rsp.payload.outputs_0,
            i_clk                      = ClockSignal("sys"),
            i_reset                    = ResetSignal("sys"),
        )
        self.platform.add_source(cfu_filename)

        # Connect CPU to the CFU:CPU bus.
        self.cpu_params.update(
            o_CfuPlugin_bus_cmd_valid                = cfu_bus.cmd.valid,
            i_CfuPlugin_bus_cmd_ready                = cfu_bus.cmd.ready,
            o_CfuPlugin_bus_cmd_payload_function_id  = cfu_bus.cmd.payload.function_id,
            o_CfuPlugin_bus_cmd_payload_inputs_0     = cfu_bus.cmd.payload.inputs_0,
            o_CfuPlugin_bus_cmd_payload_inputs_1     = cfu_bus.cmd.payload.inputs_1,
            i_CfuPlugin_bus_rsp_valid                = cfu_bus.rsp.valid,
            o_CfuPlugin_bus_rsp_ready                = cfu_bus.rsp.ready,
            i_CfuPlugin_bus_rsp_payload_outputs_0    = cfu_bus.rsp.payload.outputs_0,
        )

    def add_vfu(self, vfu_filename, rvv_source_dir):
        # Check CVFU presence.
        if not os.path.exists(vfu_filename):
            raise OSError(f"Unable to find VexRiscv VFU plugin {vfu_filename}.")

        # VFU:CPU Bus Layout.
        vfu_bus_layout = [
            ("cmd", [
                ("valid", 1),
                ("ready", 1),
                ("payload", [
                    ("instruction", 32),
                    ("inputs_0", 32),
                    ("inputs_1", 32),
                    ("rounding", 3),
                ]),
            ]),
            ("rsp", [
                ("valid", 1),
                ("ready", 1),
                ("payload", [
                    ("output", 32),
                ]),
            ]),
        ]

        # The VFU:CPU Bus.
        self.vfu_bus = vfu_bus = Record(vfu_bus_layout)

        # Connect VFU to the VFU:CPU bus.
        self.vfu_params = dict(
            i_cmd_valid                = vfu_bus.cmd.valid,
            o_cmd_ready                = vfu_bus.cmd.ready,
            i_cmd_payload_instruction  = vfu_bus.cmd.payload.instruction,
            i_cmd_payload_inputs_0     = vfu_bus.cmd.payload.inputs_0,
            i_cmd_payload_inputs_1     = vfu_bus.cmd.payload.inputs_1,
            i_cmd_payload_rounding     = vfu_bus.cmd.payload.rounding,
            o_rsp_valid                = vfu_bus.rsp.valid,
            i_rsp_ready                = vfu_bus.rsp.ready,
            o_rsp_payload_output       = vfu_bus.rsp.payload.output,
            i_clk                      = ClockSignal("sys"),
            i_reset                    = ResetSignal("sys"),
        )
        self.platform.add_source(vfu_filename)
        self.platform.add_verilog_include_path(rvv_source_dir)
        self.platform.add_verilog_include_path(str(rvv_source_dir + "/vALU"))

        # Connect CPU to the VFU:CPU bus.
        self.cpu_params.update(
            o_VfuPlugin_bus_cmd_valid                = vfu_bus.cmd.valid,
            i_VfuPlugin_bus_cmd_ready                = vfu_bus.cmd.ready,
            o_VfuPlugin_bus_cmd_payload_instruction  = vfu_bus.cmd.payload.instruction,
            o_VfuPlugin_bus_cmd_payload_inputs_0     = vfu_bus.cmd.payload.inputs_0,
            o_VfuPlugin_bus_cmd_payload_inputs_1     = vfu_bus.cmd.payload.inputs_1,
            o_VfuPlugin_bus_cmd_payload_rounding     = vfu_bus.cmd.payload.rounding,
            i_VfuPlugin_bus_rsp_valid                = vfu_bus.rsp.valid,
            o_VfuPlugin_bus_rsp_ready                = vfu_bus.rsp.ready,
            i_VfuPlugin_bus_rsp_payload_output       = vfu_bus.rsp.payload.output,
        )

        # ADD MEMORY
        self.vfu_mbus = vfu_mbus = axi.AXILiteInterface(address_width=32, data_width=32);
        self.memory_buses.append(vfu_mbus)

        self.vfu_params.update(
            o_mbus_ar_addr  = vfu_mbus.ar.addr,
            o_mbus_ar_valid = vfu_mbus.ar.valid,
            i_mbus_ar_ready = vfu_mbus.ar.ready,

            i_mbus_r_data   = vfu_mbus.r.data,
            i_mbus_r_valid  = vfu_mbus.r.valid,
            o_mbus_r_ready  = vfu_mbus.r.ready,

            o_mbus_aw_addr  = vfu_mbus.aw.addr,
            o_mbus_aw_valid = vfu_mbus.aw.valid,
            i_mbus_aw_ready = vfu_mbus.aw.ready,
            
            o_mbus_w_data   = vfu_mbus.w.data,
            o_mbus_w_strb   = vfu_mbus.w.strb,
            o_mbus_w_valid  = vfu_mbus.w.valid,

            i_mbus_b_valid  = vfu_mbus.b.valid,
            i_mbus_b_resp   = vfu_mbus.b.resp,
            o_mbus_b_ready  = vfu_mbus.b.ready,
        )


    @staticmethod
    def add_sources(platform, variant="standard"):
        cpu_filename = CPU_VARIANTS[variant] + ".v"
        vdir = get_data_mod("cpu", "vexriscv").data_location
        platform.add_source(os.path.join(vdir, cpu_filename))

    def add_soc_components(self, soc, soc_region_cls):
        # Connect Debug interface to SoC.
        if "debug" in self.variant:
            soc.bus.add_slave("vexriscv_debug", self.debug_bus, region=
                soc_region_cls(
                    origin = soc.mem_map.get("vexriscv_debug"),
                    size   = 0x100,
                    cached = False
                )
            )

        # Pass I/D Caches info to software.
        base_variant = str(self.variant.split('+')[0])
        # DCACHE is present on all variants except minimal and lite.
        if not base_variant in ["minimal", "lite"]:
            soc.add_config("CPU_HAS_DCACHE")
        # ICACHE is present on all variants except minimal.
        if not base_variant in ["minimal"]:
            soc.add_config("CPU_HAS_ICACHE")

    def use_external_variant(self, variant_filename):
        self.external_variant = True
        self.platform.add_source(variant_filename)

    def do_finalize(self):
        assert hasattr(self, "reset_address")
        if not self.external_variant:
            self.add_sources(self.platform, self.variant)
        self.specials += Instance("VexRiscv", **self.cpu_params)
        if hasattr(self, "cfu_params"):
            self.specials += Instance("Cfu", **self.cfu_params)
        if hasattr(self, "vfu_params"):
            self.specials += Instance("Vfu", **self.vfu_params)
