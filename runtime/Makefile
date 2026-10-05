EE_BIN = psxcore-runtime-bootstrap.elf
EE_OBJS = main.o transfer.o hooks.o boot_elf.o boot_sha256.o
EE_INCS = -I../../include
EE_CFLAGS = -O2 -Wall -Wextra -Werror
EE_LIBS = -liopreboot -lpatches -ldebug
EE_LINKFILE = linkfile
EE_LDFLAGS = -Wl,-Map=runtime.map
all: $(EE_BIN)
$(EE_BIN): linkfile
linkfile: $(PS2SDK)/ee/startup/linkfile resident.ld Makefile
	sed -e 's/text PT_LOAD;/resident PT_LOAD; text PT_LOAD;/' -e 's/\.text 0x00100000: {/INCLUDE resident.ld\n .text 0x00100000: {/' -e 's/_end = \. ;/_end = . ; ASSERT(. < 0x00800000, "runtime loader\/staging overlap")/' -e 's/PROVIDE(_stack = -1);/PROVIDE(_stack = 0x00f00000);/' -e 's/PROVIDE(_heap_size = -1);/PROVIDE(_heap_size = 0x100000);/' $< > $@
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
