EE_BIN = igrloader.elf
EE_OBJS = entry.o main.o cleanexec.o igr.o menu.o
EE_INCS = -I../../include
EE_CFLAGS = -G0 -Os -Wall -Wextra -Werror -DNEWLIB_PORT_AWARE
NP_IGR_RETURN_PATH ?= mc0:/BOOT/BOOT.ELF
NP_IGR_RESET_DEBUG ?= 0
EE_LINKFILE = linkfile
EE_LDFLAGS = -Wl,-Map=igrloader.map
EE_LIBS = -lpatches
.DEFAULT_GOAL := all
all: igrloader.bin igrloader_symbols.h
$(EE_BIN): linkfile
.PHONY: FORCE
FORCE:
igrloader_config.h: FORCE
	printf '#define NP_IGR_RETURN_PATH "%s"\n' '$(NP_IGR_RETURN_PATH)' > igrloader_config.tmp
	printf '#define NP_IGR_RESET_DEBUG %s\n' '$(NP_IGR_RESET_DEBUG)' >> igrloader_config.tmp
	if ! cmp -s igrloader_config.tmp $@; then cp igrloader_config.tmp $@; fi
main.o: igrloader_config.h target.h
menu.o: menu.c menu_art.h
igr.o: ../../src/igr.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
linkfile: $(PS2SDK)/ee/startup/linkfile Makefile
	sed -e 's/ENTRY(__start);/ENTRY(np_igr_loader_entry);/' -e 's/\.text 0x00100000: {/\.text 0x00084000: { KEEP(*(.igr_entry))/' -e 's/_end = \. ;/_end = . ; ASSERT(. <= 0x000e0000, "IGR loader overlaps private stack")/' -e 's/PROVIDE(_stack = -1);/PROVIDE(_stack = 0x000e0000);/' -e 's/PROVIDE(_stack_size = 128 \* 1024);/PROVIDE(_stack_size = 32 * 1024);/' -e 's/PROVIDE(_heap_size = -1);/PROVIDE(_heap_size = 0);/' $< > $@
igrloader.bin: $(EE_BIN)
	$(EE_OBJCOPY) -O binary $< $@
igrloader_symbols.h: $(EE_BIN)
	$(EE_TOOL_PREFIX)nm -n $< > igrloader.nm
	awk '$$3 == "_end" {print "#define NP_IGR_LOADER_END 0x" $$1 "u"; n++} $$3 == "np_igr_loader_entry" {print "#define NP_IGR_LOADER_ENTRY 0x" $$1 "u"; n++} $$3 == "np_igr_return_status" {print "#define NP_IGR_LOADER_STATUS 0x" $$1 "u"; n++} $$3 == "np_igr_menu_draw" {print "#define NP_IGR_MENU_DRAW 0x" $$1 "u"; n++} END {if (n != 4) exit 1}' igrloader.nm > $@
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
