IOP_BIN = ../../../build/psxcore_massproxy.irx
IOP_OBJS = main.o imports.o
IOP_OBJS_DIR = ../../../build/native-massproxy/
IOP_INCS = -I. -I../../../include
IOP_CFLAGS = -Os -Wall -Wextra -Werror -DIOMANX_OLD_NAME_ADDDELDRV=0
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
