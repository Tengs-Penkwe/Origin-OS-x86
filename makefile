MBR			:=	mbr
LOADER		:=	loader
IMAGE		:=	hd32Mi.img

ASM			:=	nasm
ASMFLAGS	:=	-f elf
LDFLAGS		:=	-I include/

CC			:=	gcc
CFLAGS		:=

LINK		:=	ld
LFLAGS		:=	-m elf_i386

DEPS		:=

OBJS		:= 

.PHONY :MBR loader kernel preprocess run clean 
MBR:
	$(ASM) $(LDFLAGS) $(MBR).S -o $(MBR).bin 

loader: MBR
	$(ASM) $(LDFLAGS) $(LOADER).S -o $(LOADER).bin 

kernel: loader
	@dd if=$(MBR).bin     of=$(IMAGE) bs=512 count=1 seek=0 conv=notrunc
	@dd if=$(LOADER).bin  of=$(IMAGE) bs=512 count=4 seek=1 conv=notrunc

preprocess: kernel
	$(ASM) $(LDFLAGS)  -E $(MBR).S -o $(MBR).xpd
	@cat -n $(MBR).xpd

run: kernel 
	@bochs -f bochsrc.mac

clean:
	@rm -rf *.o *.xpd *.bin *.out
