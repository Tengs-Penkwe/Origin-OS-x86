CURRENT_DIR	:=	$(shell pwd)
BUILD_ROOT	:=	$(CURRENT_DIR)/../

MBR			:=	mbr
LOADER		:=	loader
IMAGE		:=	$(BUILD_ROOT)/hd32Mi.img

ASM			:=	nasm
ASMFLAGS	:=	-f elf
LDFLAGS		:=	-I include/


.PHONY: MBR loader boot preprocess run clean 
MBR:
	$(ASM) $(LDFLAGS) $(MBR).S -o $(MBR).bin -O0

loader: MBR
	$(ASM) $(LDFLAGS) $(LOADER).S -o $(LOADER).bin -O0

boot: loader
	@dd if=$(MBR).bin     of=$(IMAGE) bs=512 count=1 seek=0 conv=notrunc
	@dd if=$(LOADER).bin  of=$(IMAGE) bs=512 count=4 seek=1 conv=notrunc

run: boot 
	@bochs -qf $(BUILD_ROOT)/bochsrc.mac

clean:
	@rm -rf *.o *.xpd *.bin *.out
