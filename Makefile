.DEFAULT_GOAL := library
EE_BIN = build/psxcore-inventory.elf
MODULES = iomanX fileXio ps2dev9 bdm psxcore_ro ata_bd ps2hdd_bdm ps2fs
EE_OBJS = build/probe.o build/vcd.o $(addprefix build/,$(addsuffix _irx.o,$(MODULES)))
EE_INCS = -Iinclude
EE_CFLAGS = -O2 -Wall -Wextra -Werror
EE_LIBS = -lfileXio -lpatches -ldebug
CORE_NAMES = vcd request elf boot_elf boot_sha256 compat launch patch runtime_corrections discmap sha1
CORE_OBJS = $(addprefix build/core/,$(addsuffix .o,$(CORE_NAMES)))
NP_MEMORY_ACCESS_ADAPTER ?= 1
NP_MEMORY_ACCESS_TRACE ?= 0
NP_MEMORY_RAM_FASTPATH ?= 0
NP_GP0_PACKET_COALESCE ?= 0
NP_GP0_PACKET_TRACE ?= 0
NP_GP0_SCALAR_WRITE ?= 0
NP_AUDIO_GAP_DECLICK ?= 0
NP_VIDEO_TRANSITION_FIX ?= 1
NP_RGB24_SCANOUT_ADAPTER ?= 1
NP_INTERLACE_BUFFERED_PRESENT ?= 0
NP_GPU_DMA_RASTER_SPLIT ?= 1

.PHONY: all library runtime inventory
all: library runtime
library: build/libpsxcore.a
inventory: $(EE_BIN)
runtime: | build
	mkdir -p build/runtime
	cp runtime/Makefile runtime/resident.ld runtime/main.c runtime/config.h runtime/saveguards.h runtime/dmacycles.S runtime/hooks.S runtime/transfer.S runtime/memoryaccess.c runtime/memoryaccess.h runtime/memoryaccess.S runtime/returndelay.S runtime/gtecolor.S runtime/rgb24.S runtime/gp0.S runtime/audio.S src/boot_elf.c src/boot_sha256.c src/compat.c src/compat_profiles.generated.h src/launch.c src/patch.c src/runtime_corrections.c build/runtime/
	$(MAKE) -C build/runtime NP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER) NP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE) NP_MEMORY_RAM_FASTPATH=$(NP_MEMORY_RAM_FASTPATH) NP_GP0_PACKET_COALESCE=$(NP_GP0_PACKET_COALESCE) NP_GP0_PACKET_TRACE=$(NP_GP0_PACKET_TRACE) NP_GP0_SCALAR_WRITE=$(NP_GP0_SCALAR_WRITE) NP_AUDIO_GAP_DECLICK=$(NP_AUDIO_GAP_DECLICK) NP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX) NP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER) NP_INTERLACE_BUFFERED_PRESENT=$(NP_INTERLACE_BUFFERED_PRESENT) NP_GPU_DMA_RASTER_SPLIT=$(NP_GPU_DMA_RASTER_SPLIT)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

build build/core:
	mkdir -p $@
build/core/%.o: src/%.c $(wildcard include/*.h) | build/core
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/core/compat.o: src/compat_profiles.generated.h
build/libpsxcore.a: $(CORE_OBJS)
	$(EE_AR) rcs $@ $^
build/probe.o: src/probe.c include/psxcore_vcd.h | build
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/vcd.o: src/vcd.c include/psxcore_vcd.h | build
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/%_irx.c: $(PS2SDK)/iop/irx/%.irx | build
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
build/ps2hdd_bdm_irx.c: $(PS2SDK)/iop/irx/ps2hdd-bdm.irx | build
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_bdm_irx
build/psxcore_ro.irx: iop/readonly.c iop/imports.lst iop/irx_imports.h iop/Makefile | build
	$(MAKE) -C iop
build/psxcore_ro_irx.c: build/psxcore_ro.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_ro_irx
build/%_irx.o: build/%_irx.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
