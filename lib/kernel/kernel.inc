;------------------------ Selector Definitions --------------------
TI_GDT			equ	0
RPL0			equ	0
SELECTOR_VIDEO	equ	(0x0003<<3) + TI_GDT + RPL0

;------------------------------ Address ------------------------------
KERNEL_MAP_BASE_ADDR			equ	0xc000_0000
