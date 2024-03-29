SOURCE_ROOT		= 	.
#$(shell pwd)
SOURCE_DIRS		=	$(SOURCE_ROOT)/kernel	\
					$(SOURCE_ROOT)/lib		\
					$(SOURCE_ROOT)/thread	\
					$(SOURCE_ROOT)/userprog	\
					$(SOURCE_ROOT)/fs		\
					$(SOURCE_ROOT)/device

OBJ_DIR			=	$(SOURCE_ROOT)/obj/

SRCS			=	$(shell find $(SOURCE_DIRS) -name '*.c' -o -name '*.S')
OBJS 			=	$(patsubst %,$(OBJ_DIR)%.o,$(SRCS))
DEPS			=	$(OBJS:.o=.d)

INC_DIRS		=	$(shell find $(SOURCE_DIRS) -type d)
INC_FLAGS		=	$(addprefix -I,$(INC_DIRS))
CPPFLAGS		=	$(INC_FLAGS)
#-MD -MP

ENTRY_POINT		=	0xc0001500

ASM				=	nasm
ASMFLAGS		=	-O0 -f elf -Wall

CC				=	x86_64-elf-gcc
CFLAGS			=	-O0 -m32 -Wall -fno-builtin -c -W -Wstrict-prototypes \
					-Wmissing-prototypes -nostdlib -nostartfiles 

LD				=	x86_64-elf-ld
LDFLAGS			=	-Ttext $(ENTRY_POINT) -melf_i386 -e main \
					-Map $(SOURCE_ROOT)/kernel.map 

TARGET_KERNEL	=	kernel.bin
IMAGE			=	hd32Mi.img

all: $(SOURCE_ROOT)/$(TARGET_KERNEL) tags

$(SOURCE_ROOT)/$(TARGET_KERNEL): $(OBJS) 
	$(LD) $(addprefix $(OBJ_DIR),$(notdir $(OBJS))) -o $@ $(LDFLAGS)

$(OBJ_DIR)%.c.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $(addprefix $(OBJ_DIR),$(notdir $@))

$(OBJ_DIR)%.S.o: %.S
	$(ASM) $(CPPFLAGS) $(ASMFLAGS) $< -o $(addprefix $(OBJ_DIR),$(notdir $@)) 

.PHONY: all boot kernel run clean tags doc

tags:
	@ctags -R  --exclude=html  --exclude=latex

boot: 
	make image -C boot/

kernel: $(SOURCE_ROOT)/$(TARGET_KERNEL) boot 
#	if ! [ -e hd32Mi.img ]; then \
		bximage -hd=32M  -mode="create" -q hd32Mi.img \
	fi
	dd if=$(TARGET_KERNEL) of=$(IMAGE) bs=512 count=200 seek=9 conv=notrunc

run: kernel
	bochs -qf $(SOURCE_ROOT)/bochsrc.mac
	@rm -f bochs.out

dry-run: 
	bochs -qf $(SOURCE_ROOT)/bochsrc.mac

clean:
	rm -rf *.o *.s $(OBJ_DIR)/* $(TARGET_KERNEL) cscope.* latex html

doc:
	$(shell doxygen Doxyfile)

#-include $(DEPS)
