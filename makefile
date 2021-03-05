TARGET		:=	mbr
BIN_FILE	:=	$(TARGET).bin
IMAGE		:=	hd32Mi.img

ASM			:= nasm
ASMFLAGS	:= -f elf

CC			:= gcc
CFLAGS		:=

LINK		:= ld
LDFLAGS		:= -m elf_i386

DEPS		:=

OBJS		:= 

.PHONY :build MBR clean run debug
build:
	$(ASM) $(TARGET).S -o $(BIN_FILE)
MBR: build
	@dd if=$(BIN_FILE) of=$(IMAGE) bs=512 count=1 conv=notrunc

run: build MBR 
	@bochs -f bochsrc.mac

debug:
	@bochs

clean:
	@rm -rf *.o
