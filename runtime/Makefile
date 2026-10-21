EE_BIN = psxcore.elf
.DEFAULT_GOAL := all
FILE_MODULES = iomanX fileXio ps2dev9 bdm bdmfs_fatfs ata_bd usbd usbmass_bd poweroff psxcore_powerbutton psxcore_nativegate psxcore_filebridge psxcore_iomanx bdma_usb_fs bdma_usb_bd bdma_ata_fs bdma_ata_bd psxcore_massproxy
EE_OBJS = reverbexact.o main.o startup_diag.o file_storage.o file.o file_patch.o vcd.o sha1.o transfer.o hooks.o memoryaccess.o memoryaccess_asm.o returndelay.o gtecolor.o rgb24.o gp0.o loopcycles.o codeguard_asm.o codeguard.o boot_elf.o boot_sha256.o compat.o launch.o patch.o runtime_corrections.o $(addsuffix _irx.o,$(FILE_MODULES))
EE_INCS = -I../../include
EE_OBJS += igrobserve.o
EE_OBJS += igrwait.o
EE_OBJS += igrquit.o
EE_OBJS += cdobserve.o
EE_OBJS += cdrendezvous.o
EE_OBJS += cdswap_context.o cdreopen.o cdswap_native.o cdswap.o cdswap_input.o
EE_OBJS += cdreadstatus.o
EE_OBJS += clutrefresh.o
EE_OBJS += gteintpl.o
main.o: gteintpl_patch.h
EE_OBJS += gp1idle.o
main.o: gp1idle_patch.h
main.o compat.o file_patch.o patch.o runtime_corrections.o: ../../include/psxcore_patch.h
EE_CFLAGS = -O2 -Wall -Wextra -Werror
NP_MEMORY_ACCESS_ADAPTER ?= 1
NP_MEMORY_ACCESS_TRACE ?= 0
NP_MEMORY_RAM_FASTPATH ?= 1
NP_MEMORY_ADDRESS_PRECOMPUTE ?= 1
NP_GP0_PACKET_COALESCE ?= 1
NP_GP0_PACKET_TRACE ?= 0
NP_GP0_SCALAR_WRITE ?= 1
NP_GP1_IDLE_CONTROL ?= 1
NP_GP1_EARLY_REJECT ?= 0
NP_GP0_EMPTY_FLUSH_FASTPATH ?= 0
NP_AUDIO_GAP_DECLICK ?= 1
NP_SPU_CAPTURE_IRQ ?= 1
NP_SPU_REVERB_READONLY ?= 1
NP_VIDEO_TRANSITION_FIX ?= 1
NP_RGB24_SCANOUT_ADAPTER ?= 1
NP_INTERLACE_BUFFERED_PRESENT ?= 1
NP_GPU_DMA_RASTER_SPLIT ?= 1
NP_GPU_DMA_INLINE_ACCOUNTING ?= 0
NP_MDEC_BLOCK_FASTPATH ?= 1
NP_RASTER_COST_FASTPATH ?= 1
# Include optional audio-rate feedback; launch selection defaults off.
NP_PCM_BUFFER_RATE ?= 1
# Opt-in native decoder window; full decoded-instruction validation stays on.
NP_POPS_BLOCK_LIMIT ?= 0
# Optional input/save observations, independent of native-menu return.
NP_IGR_OBSERVE ?= 0
NP_IGR_WAIT ?= 0
NP_IGR_PROBE ?= 0
NP_IGR_QUIT_PROBE ?= 0
NP_IGR_RETURN ?= 1
ifeq ($(NP_IGR_RETURN),1)
EE_OBJS += igrloader_blob.o igr.o
endif
NP_CD_OBSERVE ?= 0
NP_CD_SWAP ?= 1
NP_CD_RENDEZVOUS ?= 0
ifeq ($(NP_CD_SWAP),1)
EE_OBJS += discset.o
endif
EE_CFLAGS += -DNP_MEMORY_ACCESS_ADAPTER=$(NP_MEMORY_ACCESS_ADAPTER)
EE_CFLAGS += -DNP_MEMORY_ACCESS_TRACE=$(NP_MEMORY_ACCESS_TRACE)
EE_CFLAGS += -DNP_MEMORY_RAM_FASTPATH=$(NP_MEMORY_RAM_FASTPATH)
EE_CFLAGS += -DNP_MEMORY_ADDRESS_PRECOMPUTE=$(NP_MEMORY_ADDRESS_PRECOMPUTE)
EE_CFLAGS += -DNP_GP0_PACKET_COALESCE=$(NP_GP0_PACKET_COALESCE)
EE_CFLAGS += -DNP_GP0_PACKET_TRACE=$(NP_GP0_PACKET_TRACE)
EE_CFLAGS += -DNP_GP0_SCALAR_WRITE=$(NP_GP0_SCALAR_WRITE)
EE_CFLAGS += -DNP_GP1_IDLE_CONTROL=$(NP_GP1_IDLE_CONTROL)
EE_CFLAGS += -DNP_GP1_EARLY_REJECT=$(NP_GP1_EARLY_REJECT)
EE_CFLAGS += -DNP_GP0_EMPTY_FLUSH_FASTPATH=$(NP_GP0_EMPTY_FLUSH_FASTPATH)
EE_CFLAGS += -DNP_AUDIO_GAP_DECLICK=$(NP_AUDIO_GAP_DECLICK)
EE_CFLAGS += -DNP_SPU_CAPTURE_IRQ=$(NP_SPU_CAPTURE_IRQ)
EE_CFLAGS += -DNP_SPU_REVERB_READONLY=$(NP_SPU_REVERB_READONLY)
EE_CFLAGS += -DNP_VIDEO_TRANSITION_FIX=$(NP_VIDEO_TRANSITION_FIX)
EE_CFLAGS += -DNP_RGB24_SCANOUT_ADAPTER=$(NP_RGB24_SCANOUT_ADAPTER)
EE_CFLAGS += -DNP_INTERLACE_BUFFERED_PRESENT=$(NP_INTERLACE_BUFFERED_PRESENT)
EE_CFLAGS += -DNP_GPU_DMA_RASTER_SPLIT=$(NP_GPU_DMA_RASTER_SPLIT)
EE_CFLAGS += -DNP_GPU_DMA_INLINE_ACCOUNTING=$(NP_GPU_DMA_INLINE_ACCOUNTING)
EE_CFLAGS += -DNP_MDEC_BLOCK_FASTPATH=$(NP_MDEC_BLOCK_FASTPATH)
EE_CFLAGS += -DNP_RASTER_COST_FASTPATH=$(NP_RASTER_COST_FASTPATH)
EE_CFLAGS += -DNP_PCM_BUFFER_RATE=$(NP_PCM_BUFFER_RATE)
EE_CFLAGS += -DNP_POPS_BLOCK_LIMIT=$(NP_POPS_BLOCK_LIMIT)
EE_CFLAGS += -DNP_IGR_OBSERVE=$(NP_IGR_OBSERVE)
EE_CFLAGS += -DNP_IGR_WAIT=$(NP_IGR_WAIT)
EE_CFLAGS += -DNP_IGR_PROBE=$(NP_IGR_PROBE)
EE_CFLAGS += -DNP_IGR_QUIT_PROBE=$(NP_IGR_QUIT_PROBE)
EE_CFLAGS += -DNP_IGR_RETURN=$(NP_IGR_RETURN)
EE_CFLAGS += -DNP_CD_OBSERVE=$(NP_CD_OBSERVE)
EE_CFLAGS += -DNP_CD_RENDEZVOUS=$(NP_CD_RENDEZVOUS)
EE_CFLAGS += -DNP_CD_SWAP=$(NP_CD_SWAP)
cdswap.o cdreopen.o: EE_CFLAGS += -Os -G0 -fno-jump-tables -fno-builtin -fno-tree-loop-distribute-patterns
cdswap.o: cdswap.c cdswap.h cdreopen.h cdswap_runtime.h
cdreopen.o: cdreopen.c cdreopen.h ../../include/psxcore_cdtoc.h
file_storage.o: cdswap.h cdswap_runtime.h
main.o: cdswap_patch.h cdswap.h cdswap_bindings.h cdswap_runtime.h
cdswap_input.o: cdswap_input.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
cdswap_native.o: cdswap_native.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
cdswap_context.o: cdswap_context.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
memoryaccess.o: EE_CFLAGS += -G0 -fno-jump-tables
codeguard.o: EE_CFLAGS += -G0 -fno-jump-tables -fno-builtin -fno-tree-loop-distribute-patterns
codeguard.o: codeguard.c codeguard.h
codeguard_asm.o: codeguard.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
memoryaccess_asm.o: memoryaccess.S mdecrow.S uploadcopy.S audio.S spucapture.S spureverb.S spureverbexact.S mdec.S mdecchroma.S mdeccolor.S mdecrle.S pcmflow.S dmacycles.S rastermath.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
EE_LIBS = -lfileXio -liopreboot -lpatches -ldebug
EE_LINKFILE = linkfile
EE_LDFLAGS = -Wl,-Map=runtime.map
all: $(EE_BIN)
$(EE_BIN): linkfile
%_irx.c: $(PS2SDK)/iop/irx/%.irx
	$(PS2SDK)/bin/bin2c $< $@ $*_irx
psxcore_filebridge_irx.c: ../psxcore_filebridge.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_filebridge_irx
psxcore_nativegate_irx.c: ../psxcore_nativegate.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_nativegate_irx
psxcore_powerbutton_irx.c: ../psxcore_powerbutton.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_powerbutton_irx
psxcore_iomanx_irx.c: ../psxcore_iomanx.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_iomanx_irx
bdma_usb_fs_irx.c: ../bdma_usb_fs.irx
	$(PS2SDK)/bin/bin2c $< $@ bdma_usb_fs_irx
bdma_usb_bd_irx.c: ../bdma_usb_bd.irx
	$(PS2SDK)/bin/bin2c $< $@ bdma_usb_bd_irx
bdma_ata_fs_irx.c: ../bdma_ata_fs.irx
	$(PS2SDK)/bin/bin2c $< $@ bdma_ata_fs_irx
bdma_ata_bd_irx.c: ../bdma_ata_bd.irx
	$(PS2SDK)/bin/bin2c $< $@ bdma_ata_bd_irx
psxcore_massproxy_irx.c: ../psxcore_massproxy.irx
	$(PS2SDK)/bin/bin2c $< $@ psxcore_massproxy_irx
linkfile: $(PS2SDK)/ee/startup/linkfile resident.ld Makefile
	sed -e 's/text PT_LOAD;/resident PT_LOAD; text PT_LOAD;/' -e 's/\.text 0x00100000: {/INCLUDE resident.ld\n .text 0x00100000: {/' -e 's/_end = \. ;/_end = . ; ASSERT(. < 0x00800000, "runtime loader\/staging overlap")/' -e 's/PROVIDE(_stack = -1);/PROVIDE(_stack = 0x00f00000);/' -e 's/PROVIDE(_heap_size = -1);/PROVIDE(_heap_size = 0x100000);/' $< > $@
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

reverbexact.o: reverbexact.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -Wno-unused-variable -Wno-unused-but-set-variable -fno-strict-aliasing -fwrapv -Os -c $< -o $@
