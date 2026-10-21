.DEFAULT_GOAL := library
EE_CFLAGS = -O2 -Wall -Wextra -Werror
EE_INCS = -Iinclude
CORE_NAMES = vcd request elf boot_elf boot_sha256 compat launch file card file_patch patch runtime_corrections discmap discset sha1 igr
CORE_OBJS = $(addprefix build/core/,$(addsuffix .o,$(CORE_NAMES)))
NP_MEMORY_ACCESS_ADAPTER ?= 1
NP_MEMORY_ACCESS_TRACE ?= 0
NP_MEMORY_RAM_FASTPATH ?= 1
# Compute addresses in the existing generated call delay slots.
NP_MEMORY_ADDRESS_PRECOMPUTE ?= 1
export NP_MEMORY_ADDRESS_PRECOMPUTE
NP_GP0_PACKET_COALESCE ?= 1
NP_GPU_PACKET_FASTPATH ?= 1
NP_CD_COMMAND_ORDER ?= 1
export NP_GPU_PACKET_FASTPATH NP_CD_COMMAND_ORDER
NP_GP0_PACKET_TRACE ?= 0
NP_GP0_SCALAR_WRITE ?= 1
NP_GP1_IDLE_CONTROL ?= 1
NP_GP1_EARLY_REJECT ?= 1
NP_GP0_EMPTY_FLUSH_FASTPATH ?= 1
NP_GP0_PUBLISH_TWO_TEMPS ?= 1
export NP_GP0_PUBLISH_TWO_TEMPS
export NP_GP1_EARLY_REJECT NP_GP0_EMPTY_FLUSH_FASTPATH
NP_AUDIO_GAP_DECLICK ?= 1
NP_SPU_CAPTURE_IRQ ?= 1
NP_SPU_REVERB_READONLY ?= 1
NP_VIDEO_TRANSITION_FIX ?= 1
NP_RGB24_SCANOUT_ADAPTER ?= 1
NP_INTERLACE_BUFFERED_PRESENT ?= 1
NP_GPU_DMA_RASTER_SPLIT ?= 1
NP_GPU_DMA_INLINE_ACCOUNTING ?= 1
export NP_GPU_DMA_INLINE_ACCOUNTING
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
# Native-menu return. No save drain or new shortcut.
NP_IGR_RETURN ?= 1
NP_IGR_RETURN_PATH ?= mc0:/BOOT/BOOT.ELF
export NP_IGR_RETURN NP_IGR_RETURN_PATH
NP_CD_OBSERVE ?= 0
export NP_CD_OBSERVE
NP_CD_SWAP ?= 1
export NP_CD_SWAP
NP_CD_RENDEZVOUS ?= 0
export NP_CD_RENDEZVOUS

.PHONY: all library runtime
all: library runtime
library: build/libpsxcore.a
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
	cp runtime/Makefile runtime/resident.ld runtime/main.c runtime/config.h runtime/saveguards.h runtime/file_storage.c runtime/file_storage.h runtime/dmacycles.S runtime/hooks.S runtime/transfer.S runtime/memoryaccess.c runtime/memoryaccess.h runtime/memoryaccess.S runtime/uploadcopy.S runtime/returndelay.S runtime/gtecolor.S runtime/rgb24.S runtime/gp0.S runtime/loopcycles.S runtime/codeguard.S runtime/codeguard.c runtime/codeguard.h runtime/audio.S runtime/spucapture.S runtime/spureverb.S runtime/spureverbexact.S runtime/reverbexact.c runtime/mdec.S runtime/mdecrow.S runtime/mdecchroma.S runtime/mdeccolor.S runtime/mdecrle.S runtime/pcmflow.S runtime/rastermath.S src/boot_elf.c src/boot_sha256.c src/compat.c src/compat_profiles.generated.h src/launch.c src/file.c src/file_patch.c src/vcd.c src/sha1.c src/patch.c src/runtime_corrections.c build/runtime/
	cp runtime/igrobserve.S runtime/igrobserve.h build/runtime/
	cp runtime/startup_diag.c runtime/startup_diag.h build/runtime/
	cp runtime/igrwait.S runtime/igrwait.h build/runtime/
	cp runtime/igrquit.S runtime/igrquit.h build/runtime/
	cp runtime/igrreturn.h runtime/igrmenu.h build/runtime/
ifeq ($(NP_IGR_RETURN),1)
	cp src/elf.c src/igr.c runtime/igrloader/igrloader_symbols.h build/runtime/
	$(PS2SDK)/bin/bin2c runtime/igrloader/igrloader.bin build/runtime/igrloader_blob.c np_igr_loader_blob
endif
	cp runtime/cdobserve.S runtime/cdobserve.h build/runtime/
	cp runtime/packet_fast.S runtime/cdorder.c runtime/cdorder.S build/runtime/
	cp runtime/cdrendezvous.S runtime/cdrendezvous.h build/runtime/
	cp src/discset.c build/runtime/
	cp runtime/cdreopen.c runtime/cdreopen.h runtime/cdswap.c runtime/cdswap.h runtime/cdswap_input.S runtime/cdswap_patch.h runtime/cdswap_runtime.h runtime/cdswap_bindings.h runtime/cdswap_native.S runtime/cdswap_context.S build/runtime/
	cp runtime/cdreadstatus.S build/runtime/
	cp runtime/clutrefresh.S build/runtime/
	cp runtime/gteintpl.S runtime/gteintpl_patch.h build/runtime/
	cp runtime/gp1idle.S runtime/gp1idle_patch.h build/runtime/
	$(MAKE) -C build/runtime NP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER) NP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE) NP_MEMORY_RAM_FASTPATH=$(NP_MEMORY_RAM_FASTPATH) NP_GP0_PACKET_COALESCE=$(NP_GP0_PACKET_COALESCE) NP_GP0_PACKET_TRACE=$(NP_GP0_PACKET_TRACE) NP_GP0_SCALAR_WRITE=$(NP_GP0_SCALAR_WRITE) NP_GP1_IDLE_CONTROL=$(NP_GP1_IDLE_CONTROL) NP_AUDIO_GAP_DECLICK=$(NP_AUDIO_GAP_DECLICK) NP_SPU_CAPTURE_IRQ=$(NP_SPU_CAPTURE_IRQ) NP_SPU_REVERB_READONLY=$(NP_SPU_REVERB_READONLY) NP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX) NP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER) NP_INTERLACE_BUFFERED_PRESENT=$(NP_INTERLACE_BUFFERED_PRESENT) NP_GPU_DMA_RASTER_SPLIT=$(NP_GPU_DMA_RASTER_SPLIT) NP_MDEC_BLOCK_FASTPATH=$(NP_MDEC_BLOCK_FASTPATH) NP_RASTER_COST_FASTPATH=$(NP_RASTER_COST_FASTPATH) NP_PCM_BUFFER_RATE=$(NP_PCM_BUFFER_RATE) NP_POPS_BLOCK_LIMIT=$(NP_POPS_BLOCK_LIMIT)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

build build/core:
	mkdir -p $@
build/core/%.o: src/%.c $(wildcard include/*.h) | build/core
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
build/core/compat.o: src/compat_profiles.generated.h
build/libpsxcore.a: $(CORE_OBJS)
	$(EE_AR) rcs $@ $^
