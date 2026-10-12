IOP_BIN = ../../../build/psxcore_fatfs.irx
IOP_OBJS = main.o driver.o imports.o diskio.o ff.o ffsystem.o ffunicode.o volume.o boot_sha256.o
IOP_OBJS_DIR = ../../../build/private-fatfs/
NP_PS2SDK_SOURCE ?= /src/libs/ps2sdk
FATFS_SOURCE = $(NP_PS2SDK_SOURCE)/iop/fs/bdmfs_fatfs/src
FATFS = $(NP_PS2SDK_SOURCE)/common/external_deps/fatfs/source
IOP_INCS = -I. -I../../../include -I$(FATFS_SOURCE) -I$(FATFS_SOURCE)/include -I$(FATFS)/include
IOP_CFLAGS = -Wall -Werror -DIOMANX_OLD_NAME_ADDDELDRV=0 -DIOMANX_OLD_NAME_COMPATIBILITY=1
IOP_LIBS = -lgcc
all: $(IOP_BIN)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal
$(IOP_OBJS_DIR)driver.o: $(FATFS_SOURCE)/fs_driver.c
$(IOP_OBJS_DIR)volume.o: ../../../src/volume.c ../../../include/psxcore_volume.h
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
$(IOP_OBJS_DIR)boot_sha256.o: ../../../src/boot_sha256.c ../../../include/psxcore_hash.h
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
$(IOP_OBJS_DIR)main.o: $(FATFS_SOURCE)/main.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
$(IOP_OBJS_DIR)diskio.o $(IOP_OBJS_DIR)ff.o $(IOP_OBJS_DIR)ffsystem.o $(IOP_OBJS_DIR)ffunicode.o: $(IOP_OBJS_DIR)%.o: $(FATFS)/%.c
	$(DIR_GUARD)
	$(IOP_C_COMPILE) -c $< -o $@
