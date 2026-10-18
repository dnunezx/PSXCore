IOP_BIN = ../../build/psxcore_filebridge.irx
.DEFAULT_GOAL := all
IOP_OBJS = bridge.o native.o worker.o file.o vcd.o boot_sha256.o imports.o
IOP_OBJS_DIR = ../../build/filebridge/
IOP_INCS = -I../../include
IOP_CFLAGS = -O2 -Wall -Wextra -Werror -DNP_FILE_IO_ONLY -DIOMANX_OLD_NAME_ADDDELDRV=0
NP_CD_SWAP ?= 1
IOP_CFLAGS += -DNP_CD_SWAP=$(NP_CD_SWAP)
ifeq ($(NP_CD_SWAP),1)
IOP_OBJS += discset.o
endif
$(IOP_OBJS_DIR)discset.o: ../../src/discset.c ../../include/psxcore_discset.h ../../include/psxcore_cdtoc.h
	$(IOP_CC) $(IOP_CFLAGS) $(IOP_INCS) -c $< -o $@
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
$(IOP_OBJS_DIR)bridge.o $(IOP_OBJS_DIR)file.o: ../../include/psxcore_file.h ../../include/psxcore_launch.h ../../include/psxcore_vcd.h
$(IOP_OBJS_DIR)bridge.o $(IOP_OBJS_DIR)native.o $(IOP_OBJS_DIR)imports.o: native.h irx_imports.h ../../include/psxcore_nativegate.h
$(IOP_OBJS_DIR)bridge.o $(IOP_OBJS_DIR)native.o $(IOP_OBJS_DIR)worker.o: worker.h
$(IOP_OBJS_DIR)worker.o: irx_imports.h ../../include/psxcore_nativegate.h
$(IOP_OBJS_DIR)file.o: ../../src/file.c
	$(IOP_CC) $(IOP_CFLAGS) $(IOP_INCS) -c $< -o $@
$(IOP_OBJS_DIR)vcd.o: ../../src/vcd.c
	$(IOP_CC) $(IOP_CFLAGS) $(IOP_INCS) -c $< -o $@
$(IOP_OBJS_DIR)boot_sha256.o: ../../src/boot_sha256.c ../../include/psxcore_boot.h ../../include/psxcore_hash.h
	$(IOP_CC) $(IOP_CFLAGS) $(IOP_INCS) -c $< -o $@

$(IOP_OBJS_DIR)bridge.o: disc_tail.h
ifeq ($(NP_CD_SWAP),1)
$(IOP_OBJS_DIR)bridge.o: ../../include/psxcore_discset.h ../../include/psxcore_cdtoc.h
endif
