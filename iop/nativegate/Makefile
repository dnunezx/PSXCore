IOP_BIN = ../../build/psxcore_nativegate.irx
IOP_OBJS = gate.o exports.o imports.o
IOP_OBJS_DIR = ../../build/nativegate/
IOP_INCS = -I../../include
IOP_CFLAGS = -O2 -Wall -Wextra -Werror
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
$(IOP_OBJS_DIR)gate.o: ../../include/psxcore_nativegate.h irx_imports.h
$(IOP_OBJS_DIR)imports.o: irx_imports.h
