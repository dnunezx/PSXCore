IOP_BIN = ../../../build/psxcore_iomanx.irx
IOP_OBJS = manager.o exports.o imports.o
IOP_OBJS_DIR = ../../../build/private-iomanx/
# Source from the SDK used to build the other storage modules. No firmware.
NP_PS2SDK_SOURCE ?= /src/libs/ps2sdk
IOMANX_SOURCE = $(NP_PS2SDK_SOURCE)/iop/system/iomanx/src
IOP_INCS = -I$(IOMANX_SOURCE)
IOP_CFLAGS = -O2 -Wall -Werror -DIOMANX_OLD_NAME_ADDDELDRV=0 -DIOMANX_OLD_NAME_COMPATIBILITY=0 -DIOMANX_ENABLE_LEGACY_IOMAN_HOOK -DIOMAN_USE_FILE_STRUCT_TEMP_STACK
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
$(IOP_OBJS_DIR)manager.o: $(IOMANX_SOURCE)/iomanX.c
$(IOP_OBJS_DIR)exports.o: $(IOMANX_SOURCE)/exports.tab
