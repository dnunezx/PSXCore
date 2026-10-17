IOP_BIN = ../../build/psxcore_powerbutton.irx
IOP_OBJS = button.o imports.o
IOP_OBJS_DIR = ../../build/powerbutton/
IOP_CFLAGS = -O2 -Wall -Wextra -Werror
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
$(IOP_OBJS_DIR)button.o: irx_imports.h
$(IOP_OBJS_DIR)imports.o: irx_imports.h
