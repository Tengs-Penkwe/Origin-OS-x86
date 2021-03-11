#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"

//Currently, we have 0x21 inerrupts
#define IDT_DESC_CNT	0x21

#define PIC_M_CRTL		0x20
#define PIC_M_DATA		0x21
#define PIC_S_CRTL		0xa0
#define PIC_S_DATA		0xa1


/*** Interrupt Gate***/
struct gate_desc{
	uint16_t	func_offset_low_word;
	uint16_t	selector;
	uint8_t		dcount;			//Fixed for Interrupt Gate
	uint8_t		attribute;
	uint16_t	func_offset_high_word;
};

//Array of Interrupt Gate
static struct gate_desc idt[IDT_DESC_CNT];
/// Interruption Address and Handler Function
extern intr_handler intr_entry_table[IDT_DESC_CNT]; 

/** Initialize 8259A **/
static void pic_init(){
	//Master
	outb (PIC_M_CRTL,0x11);		//ICW1: Edge trigger, cascade,
	outb (PIC_M_DATA,0x20);		//ICW2: Initial Int Number
	outb (PIC_M_DATA,0x04);		//ICW3: IR2 links to slave
	outb (PIC_M_DATA,0x01);		//ICW4: 8086, normal EOI
	//Slave
	outb (PIC_S_CRTL,0x11);
	outb (PIC_S_DATA,0x28);
	outb (PIC_S_DATA,0x02);
	outb (PIC_S_DATA,0x01);
	
	/* open IR0 in master, only clock int is allowed */
	outb (PIC_M_DATA,0xfe);
	outb (PIC_S_DATA,0xff);
	
	put_str("    pic_init()done\n");
}


static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function){
	p_gdesc -> func_offset_high_word = (uint32_t)function & 0x0000FFFF;
	p_gdesc -> selector = SELECTOR_K_CODE;
	p_gdesc -> dcount = 0;
	p_gdesc -> attribute = attr;
	p_gdesc -> func_offset_low_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

static void idt_desc_init(void){
	for (int i=0; i < IDT_DESC_CNT; i++){
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	put_str("    idt_desc_init() done.\n");
}

void idt_init(void){
	put_str("idt_init start\n");
	idt_desc_init();
	pic_init();

	uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)(uint32_t)idt<<16));
	asm volatile ("lidt %0;"::"m"(idt_operand));
	put_str("idt_init done.\n");
}
