BUILD_ROOT		= 	$(shell pwd)
BUILD_DIR		=	$(BUILD_ROOT)/boot/	\
					$(BUILD_ROOT)/kernel

.PHONY: boot kernel run clean

boot:
	@make boot -C boot/

kernel: boot
	@make kernel -C kernel/

run: boot kernel
	@bochs -qf $(BUILD_ROOT)/bochsrc.mac

clean:
	@for dir in $(BUILD_DIR); \
	do \
		make clean -C $$dir; \
	done

