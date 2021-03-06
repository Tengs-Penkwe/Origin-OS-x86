SOURCE_ROOT		= 	.
#$(shell pwd)
SOURCE_DIRS		=	$(SOURCE_ROOT)/kernel	\
					$(SOURCE_ROOT)/lib 

OBJ_DIR			=	$(SOURCE_ROOT)/obj

SRCS			=	$(shell find $(SOURCE_DIRS) -name '*.c' -o -name '*.S')
OBJS 			=	$(patsubst %,$(OBJ_DIR)/%.o,$(SRCS))
DEPS			=	$(OBJS:.o=.d)

INC_DIRS		=	$(shell find $(SOURCE_DIRS) -type d)
INC_FLAGS		=	$(addprefix -I,$(INC_DIRS))

CPPFLAGS		=	$(INC_FLAGS) -MD -MP

ASM				=	nasm
ASMFLAGS		=	-f elf

CC				=	x86_64-elf-gcc
CFLAGS			=	-m32 -z nognustack

LD				=	x86_64-elf-ld
LDFLAGS			=	-Ttext 0xc0001500 -m elf_i386 -z stack-size=0  

TARGET_KERNEL	=	kernel.bin
IMAGE			=	hd32Mi.img

$(SOURCE_ROOT)/$(TARGET_KERNEL): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ 

$(OBJ_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(ASM) $(CPPFLAGS) $(ASMFLAGS) $< -o $@ 

boot:
	make boot -C boot/

lib:
	make lib -C lib/

kernel: 
	make kernel -C kernel/

.PHONY:  image run clean

image: boot lib kernel
	if ! [ -e hd32Mi.img ]; \
		then \
		bximage -hd=32M  -mode="create" -q hd32Mi.img \ 
	fi
	make image -C boot/
	dd if=$(TARGET_KERNEL) of=$(IMAGE) bs=512 count=200 seek=9 conv=notrunc

run: boot image
	bochs -qf $(SOURCE_ROOT)/bochsrc.mac

clean:
	for dir in $(SOURCE_DIRS); \
	do \
		make clean -C $$dir; \
	done
	rm -rf $(OBJ_DIR)

-include $(DEPS)
