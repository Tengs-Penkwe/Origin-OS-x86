SOURCE_ROOT		= 	.
#$(shell pwd)
SOURCE_DIRS		=	$(SOURCE_ROOT)/kernel	\
					$(SOURCE_ROOT)/lib

OBJ_DIR			=	$(SOURCE_ROOT)/obj/

SRCS			=	$(shell find $(SOURCE_DIRS) -name '*.c' -o -name '*.S')
OBJS 			=	$(patsubst %,$(OBJ_DIR)%.o,$(SRCS))
DEPS			=	$(OBJS:.o=.d)

INC_DIRS		=	$(shell find $(SOURCE_DIRS) -type d)
INC_FLAGS		=	$(addprefix -I,$(INC_DIRS))
CPPFLAGS		=	$(INC_FLAGS) -MD -MP

ASM				=	nasm
ASMFLAGS		=	-f elf

CC				=	x86_64-elf-gcc
CFLAGS			=	-m32 -fno-builtin -z nognustack

LD				=	x86_64-elf-ld
LDFLAGS			=	-Ttext 0xc0001500 -m elf_i386 -e main  

TARGET_KERNEL	=	kernel.bin
IMAGE			=	hd32Mi.img

$(SOURCE_ROOT)/$(TARGET_KERNEL): $(OBJS)
	$(LD) $(addprefix $(OBJ_DIR),$(notdir $(OBJS))) -o $@ $(LDFLAGS)

$(OBJ_DIR)%.c.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $(addprefix $(OBJ_DIR),$(notdir $@))

$(OBJ_DIR)%.S.o: %.S
	$(ASM) $(CPPFLAGS) $(ASMFLAGS) $< -o $(addprefix $(OBJ_DIR),$(notdir $@)) 

boot:
	make boot -C boot/

.PHONY:  image disas run clean

image: $(SOURCE_ROOT)/$(TARGET_KERNEL) boot
#	if ! [ -e hd32Mi.img ]; then \
		bximage -hd=32M  -mode="create" -q hd32Mi.img \
	fi
	make image -C boot/
	dd if=$(TARGET_KERNEL) of=$(IMAGE) bs=512 count=200 seek=9 conv=notrunc

disas:

run: boot image
	bochs -qf $(SOURCE_ROOT)/bochsrc.mac

clean:
	for dir in $(SOURCE_DIRS); \
	do \
		make clean -C $$dir; \
	done
	rm -rf *.o *.s $(OBJ_DIR)/* $(TARGET_KERNEL)

#-include $(DEPS)
