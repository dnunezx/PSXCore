EE_BIN = psxcore-runtime-bootstrap.elf
.DEFAULT_GOAL := all
EE_OBJS = main.o transfer.o hooks.o memoryaccess.o memoryaccess_asm.o rgb24.o boot_elf.o boot_sha256.o compat.o launch.o patch.o runtime_corrections.o
EE_INCS = -I../../include
EE_CFLAGS = -O2 -Wall -Wextra -Werror
NP_MEMORY_ACCESS_ADAPTER ?= 0
NP_MEMORY_ACCESS_TRACE ?= 0
NP_VIDEO_TRANSITION_FIX ?= 0
NP_RGB24_SCANOUT_ADAPTER ?= 0
EE_CFLAGS += -DNP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER)
EE_CFLAGS += -DNP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE)
EE_CFLAGS += -DNP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX)
EE_CFLAGS += -DNP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER)
memoryaccess.o: EE_CFLAGS += -G0 -fno-jump-tables
memoryaccess_asm.o: memoryaccess.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
EE_LIBS = -liopreboot -lpatches -ldebug
EE_LINKFILE = linkfile
EE_LDFLAGS = -Wl,-Map=runtime.map
all: $(EE_BIN)
$(EE_BIN): linkfile
linkfile: $(PS2SDK)/ee/startup/linkfile resident.ld Makefile
	sed -e 's/text PT_LOAD;/resident PT_LOAD; text PT_LOAD;/' -e 's/\.text 0x00100000: {/INCLUDE resident.ld\n .text 0x00100000: {/' -e 's/_end = \. ;/_end = . ; ASSERT(. < 0x00800000, "runtime loader\/staging overlap")/' -e 's/PROVIDE(_stack = -1);/PROVIDE(_stack = 0x00f00000);/' -e 's/PROVIDE(_heap_size = -1);/PROVIDE(_heap_size = 0x100000);/' $< > $@
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
