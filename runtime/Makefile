EE_BIN = psxcore-runtime-bootstrap.elf
.DEFAULT_GOAL := all
FILE_MODULES = iomanX fileXio ps2dev9 bdm bdmfs_fatfs ata_bd psxcore_filebridge
EE_OBJS = main.o file_storage.o file.o file_patch.o vcd.o sha1.o transfer.o hooks.o memoryaccess.o memoryaccess_asm.o returndelay.o gtecolor.o rgb24.o gp0.o loopcycles.o boot_elf.o boot_sha256.o compat.o launch.o patch.o runtime_corrections.o $(addsuffix _irx.o,$(FILE_MODULES))
EE_INCS = -I../../include
EE_CFLAGS = -O2 -Wall -Wextra -Werror
NP_MEMORY_ACCESS_ADAPTER ?= 1
NP_MEMORY_ACCESS_TRACE ?= 0
NP_MEMORY_RAM_FASTPATH ?= 1
NP_GP0_PACKET_COALESCE ?= 1
NP_GP0_PACKET_TRACE ?= 0
NP_GP0_SCALAR_WRITE ?= 1
NP_AUDIO_GAP_DECLICK ?= 1
NP_SPU_CAPTURE_IRQ ?= 1
NP_SPU_REVERB_READONLY ?= 1
NP_VIDEO_TRANSITION_FIX ?= 1
NP_RGB24_SCANOUT_ADAPTER ?= 1
NP_INTERLACE_BUFFERED_PRESENT ?= 1
NP_GPU_DMA_RASTER_SPLIT ?= 1
EE_CFLAGS += -DNP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER)
EE_CFLAGS += -DNP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE)
EE_CFLAGS += -DNP_MEMORY_RAM_FASTPATH=$(NP_MEMORY_RAM_FASTPATH)
EE_CFLAGS += -DNP_GP0_PACKET_COALESCE=$(NP_GP0_PACKET_COALESCE)
EE_CFLAGS += -DNP_GP0_PACKET_TRACE=$(NP_GP0_PACKET_TRACE)
EE_CFLAGS += -DNP_GP0_SCALAR_WRITE=$(NP_GP0_SCALAR_WRITE)
EE_CFLAGS += -DNP_AUDIO_GAP_DECLICK=$(NP_AUDIO_GAP_DECLICK)
EE_CFLAGS += -DNP_SPU_CAPTURE_IRQ=$(NP_SPU_CAPTURE_IRQ)
EE_CFLAGS += -DNP_SPU_REVERB_READONLY=$(NP_SPU_REVERB_READONLY)
EE_CFLAGS += -DNP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX)
EE_CFLAGS += -DNP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER)
EE_CFLAGS += -DNP_INTERLACE_BUFFERED_PRESENT=$(NP_INTERLACE_BUFFERED_PRESENT)
EE_CFLAGS += -DNP_GPU_DMA_RASTER_SPLIT=$(NP_GPU_DMA_RASTER_SPLIT)
memoryaccess.o: EE_CFLAGS += -G0 -fno-jump-tables
memoryaccess_asm.o: memoryaccess.S audio.S spucapture.S spureverb.S dmacycles.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
EE_LIBS = -lfileXio -liopreboot -lpatches -ldebug
EE_LINKFILE = linkfile
EE_LDFLAGS = -Wl,-Map=runtime.map
all: $(EE_BIN)
$(EE_BIN): linkfile
%_irx.c: $(PS2SDK)/iop/irx/%.irx
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
psxcore_filebridge_irx.c: ../psxcore_filebridge.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_filebridge_irx
linkfile: $(PS2SDK)/ee/startup/linkfile resident.ld Makefile
	sed -e 's/text PT_LOAD;/resident PT_LOAD; text PT_LOAD;/' -e 's/\.text 0x00100000: {/INCLUDE resident.ld\n .text 0x00100000: {/' -e 's/_end = \. ;/_end = . ; ASSERT(. < 0x00800000, "runtime loader\/staging overlap")/' -e 's/PROVIDE(_stack = -1);/PROVIDE(_stack = 0x00f00000);/' -e 's/PROVIDE(_heap_size = -1);/PROVIDE(_heap_size = 0x100000);/' $< > $@
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
