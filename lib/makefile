CURRENT_DIR	:=	$(shell pwd)
BUILD_ROOT	:=	$(CURRENT_DIR)/../

IMAGE		:=	$(BUILD_ROOT)/hd32Mi.img

TARGET		:=	lib

ASM			:=	nasm
ASMFLAGS	:=	-f elf
LDFLAGS		:=	-I include/

CC			:=	x86_64-elf-gcc
CFLAGS		:=	-m32 -z nognustack

.PHONY: 

lib:
	$(CC) $(CFLAGS) -c $(OBJ).c -o $(OBJ).o

clean:
	@rm -rf *.o *.xpd *.bin *.out
