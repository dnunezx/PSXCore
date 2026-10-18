.DEFAULT_GOAL := library
EE_BIN = build/psxcore-inventory.elf
MODULES = iomanX fileXio ps2dev9 bdm psxcore_ro ata_bd ps2hdd_bdm ps2fs
EE_OBJS = build/probe.o build/vcd.o $(addprefix build/,$(addsuffix _irx.o,$(MODULES)))
EE_INCS = -Iinclude
EE_CFLAGS = -O2 -Wall -Wextra -Werror
EE_LIBS = -lfileXio -lpatches -ldebug
CORE_NAMES = vcd request elf boot_elf boot_sha256 compat launch file card file_patch patch runtime_corrections discmap sha1 igr
CORE_OBJS = $(addprefix build/core/,$(addsuffix .o,$(CORE_NAMES)))
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
NP_MDEC_BLOCK_FASTPATH ?= 1
NP_RASTER_COST_FASTPATH ?= 1
# Include optional audio-rate feedback; launch selection defaults off.
NP_PCM_BUFFER_RATE ?= 1
NP_POPS_BLOCK_LIMIT ?= 0
# Opt-in guest observations only; does not enable input masking or runtime return.
NP_IGR_OBSERVE ?= 0
export NP_IGR_OBSERVE
# Dormant pinned-runtime wait primitive; no pause hook or IGR enablement.
NP_IGR_WAIT ?= 0
export NP_IGR_WAIT
NP_IGR_PROBE ?= 0
export NP_IGR_PROBE
NP_IGR_QUIT_PROBE ?= 0
export NP_IGR_QUIT_PROBE
# Experimental native-menu return. No save drain or new shortcut.
NP_IGR_RETURN ?= 0
NP_IGR_RETURN_PATH ?= mc0:/BOOT/BOOT.ELF
export NP_IGR_RETURN NP_IGR_RETURN_PATH
NP_CD_OBSERVE ?= 0
export NP_CD_OBSERVE
NP_CD_RENDEZVOUS ?= 0
export NP_CD_RENDEZVOUS
# Dormant reply planner only; no controller hooks or media transitions.
NP_CD_PROTOCOL ?= 0
export NP_CD_PROTOCOL
# Dormant native reply ownership; no producer lease, command hook or swap.
NP_CD_RESPONSE ?= 0
export NP_CD_RESPONSE
# Dormant command/effects binding in an isolated, bounded resident arena.
NP_CD_CONTROL ?= 0
export NP_CD_CONTROL
NP_CD_SESSION ?= 0
export NP_CD_SESSION
NP_CD_PARK ?= 0
export NP_CD_PARK
NP_CD_GATE ?= 0
export NP_CD_GATE
NP_CD_CONTEXT ?= 0
export NP_CD_CONTEXT

.PHONY: all library runtime inventory
all: library runtime
library: build/libpsxcore.a
inventory: $(EE_BIN)
NP_BDMA_MODULE_DIR ?= ../psxcore-checks/popstarter-usb-reference-20261011/popsloader-drivers
runtime: | build
	$(if $(filter 1,$(NP_IGR_RETURN)),$(MAKE) -C runtime/igrloader,)
	$(MAKE) -C iop/nativegate
	$(MAKE) -C iop/storage/iomanx
	$(MAKE) -C iop/storage/nativeusb
	$(MAKE) -C iop/powerbutton
	printf '%s  %s\n' 5ea4818ba1cf5207f6d7cadb4c13b5afa88c37c260750c21155b079a8c18f369 '$(NP_BDMA_MODULE_DIR)/usbd.irx.usbexfat' | sha256sum -c -
	printf '%s  %s\n' 70a369e47745e7f995fa771edbbfed086727b759fba915a7070c0fbcf061dd34 '$(NP_BDMA_MODULE_DIR)/usbhdfsd.irx.usbexfat' | sha256sum -c -
	printf '%s  %s\n' 4e1d39365854747117a08f8a8d843937614590ea072cf2b70f6e4c2ebbf3c6ee '$(NP_BDMA_MODULE_DIR)/usbd.irx.ata' | sha256sum -c -
	printf '%s  %s\n' 1cc865cd09997bd708e9e102c0f0f1f17f352b7844b282fcfa6a00f38609db9c '$(NP_BDMA_MODULE_DIR)/usbhdfsd.irx.ata' | sha256sum -c -
	cp $(NP_BDMA_MODULE_DIR)/usbd.irx.usbexfat build/bdma_usb_fs.irx
	cp $(NP_BDMA_MODULE_DIR)/usbhdfsd.irx.usbexfat build/bdma_usb_bd.irx
	cp $(NP_BDMA_MODULE_DIR)/usbd.irx.ata build/bdma_ata_fs.irx
	cp $(NP_BDMA_MODULE_DIR)/usbhdfsd.irx.ata build/bdma_ata_bd.irx
	$(MAKE) -C iop/filebridge
	mkdir -p build/runtime
	cp runtime/Makefile runtime/resident.ld runtime/main.c runtime/config.h runtime/saveguards.h runtime/file_storage.c runtime/file_storage.h runtime/dmacycles.S runtime/hooks.S runtime/transfer.S runtime/memoryaccess.c runtime/memoryaccess.h runtime/memoryaccess.S runtime/returndelay.S runtime/gtecolor.S runtime/rgb24.S runtime/gp0.S runtime/loopcycles.S runtime/codeguard.S runtime/codeguard.c runtime/codeguard.h runtime/audio.S runtime/spucapture.S runtime/spureverb.S runtime/spureverbexact.S runtime/reverbexact.c runtime/mdec.S runtime/mdecchroma.S runtime/mdeccolor.S runtime/mdecrle.S runtime/pcmflow.S runtime/rastermath.S src/boot_elf.c src/boot_sha256.c src/compat.c src/compat_profiles.generated.h src/launch.c src/file.c src/file_patch.c src/vcd.c src/sha1.c src/patch.c src/runtime_corrections.c build/runtime/
	cp runtime/igrobserve.S runtime/igrobserve.h build/runtime/
	cp runtime/startup_diag.c runtime/startup_diag.h build/runtime/
	cp runtime/igrwait.S runtime/igrwait.h build/runtime/
	cp runtime/igrquit.S runtime/igrquit.h build/runtime/
	cp runtime/igrreturn.h build/runtime/
ifeq ($(NP_IGR_RETURN),1)
	cp src/elf.c src/igr.c runtime/igrloader/igrloader_symbols.h build/runtime/
	$(PS2SDK)/bin/bin2c runtime/igrloader/igrloader.bin build/runtime/igrloader_blob.c np_igr_loader_blob
endif
	cp runtime/cdobserve.S runtime/cdobserve.h build/runtime/
	cp runtime/cdrendezvous.S runtime/cdrendezvous.h build/runtime/
	cp runtime/cdprotocol.c runtime/cdprotocol.h build/runtime/
	cp runtime/cdresponse.c runtime/cdresponse.h runtime/cdresponse_native.S build/runtime/
	cp runtime/cdcontrol.c runtime/cdcontrol.h build/runtime/
	cp runtime/cdsession.c runtime/cdsession.h build/runtime/
	cp runtime/cdpark.c runtime/cdpark.h build/runtime/
	cp runtime/cdgate.c runtime/cdgate.h runtime/cdgate_patch.h runtime/cdgate_native.S build/runtime/
	cp runtime/cdcontext.S runtime/cdcontext.h build/runtime/
	cp runtime/cdreadstatus.S build/runtime/
	cp runtime/clutrefresh.S build/runtime/
	$(MAKE) -C build/runtime NP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER) NP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE) NP_MEMORY_RAM_FASTPATH=$(NP_MEMORY_RAM_FASTPATH) NP_GP0_PACKET_COALESCE=$(NP_GP0_PACKET_COALESCE) NP_GP0_PACKET_TRACE=$(NP_GP0_PACKET_TRACE) NP_GP0_SCALAR_WRITE=$(NP_GP0_SCALAR_WRITE) NP_AUDIO_GAP_DECLICK=$(NP_AUDIO_GAP_DECLICK) NP_SPU_CAPTURE_IRQ=$(NP_SPU_CAPTURE_IRQ) NP_SPU_REVERB_READONLY=$(NP_SPU_REVERB_READONLY) NP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX) NP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER) NP_INTERLACE_BUFFERED_PRESENT=$(NP_INTERLACE_BUFFERED_PRESENT) NP_GPU_DMA_RASTER_SPLIT=$(NP_GPU_DMA_RASTER_SPLIT) NP_MDEC_BLOCK_FASTPATH=$(NP_MDEC_BLOCK_FASTPATH) NP_RASTER_COST_FASTPATH=$(NP_RASTER_COST_FASTPATH) NP_PCM_BUFFER_RATE=$(NP_PCM_BUFFER_RATE) NP_POPS_BLOCK_LIMIT=$(NP_POPS_BLOCK_LIMIT)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

build build/core:
	mkdir -p $@
build/core/%.o: src/%.c $(wildcard include/*.h) | build/core
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/core/compat.o: src/compat_profiles.generated.h
build/libpsxcore.a: $(CORE_OBJS)
	$(EE_AR) rcs $@ $^
build/probe.o: src/probe.c include/psxcore_vcd.h | build
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/vcd.o: src/vcd.c include/psxcore_vcd.h | build
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/%_irx.c: $(PS2SDK)/iop/irx/%.irx | build
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
build/ps2hdd_bdm_irx.c: $(PS2SDK)/iop/irx/ps2hdd-bdm.irx | build
	$(PS2SDK)/bin/bin2c $< $@ ps2hdd_bdm_irx
build/psxcore_ro.irx: iop/readonly.c iop/imports.lst iop/irx_imports.h iop/Makefile | build
	$(MAKE) -C iop
build/psxcore_ro_irx.c: build/psxcore_ro.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_ro_irx
build/%_irx.o: build/%_irx.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
