CURRENT_DIR	:=	$(shell pwd)
BUILD_ROOT	:=	$(CURRENT_DIR)/../

MBR			:=	mbr
LOADER		:=	loader
IMAGE		:=	$(BUILD_ROOT)/hd32Mi.img

ASM			:=	nasm
ASMFLAGS	:=	-f elf
LDFLAGS		:=	-I include/


.PHONY: boot image clean 
boot:
	$(ASM) $(LDFLAGS) $(MBR).S -o $(MBR).bin -O0
	$(ASM) $(LDFLAGS) $(LOADER).S -o $(LOADER).bin -O0

image:
	@dd if=$(MBR).bin     of=$(IMAGE) bs=512 count=1 seek=0 conv=notrunc
	@dd if=$(LOADER).bin  of=$(IMAGE) bs=512 count=4 seek=1 conv=notrunc

clean:
	@rm -rf *.o *.xpd *.bin *.out
