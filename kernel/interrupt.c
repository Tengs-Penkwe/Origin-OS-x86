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

/// Array of Interrupt Gate
static struct gate_desc idt[IDT_DESC_CNT];
/// Name of Interrupt
char* intr_name[IDT_DESC_CNT];

/// Address of Handler Function
extern intr_handler intr_entry_table[IDT_DESC_CNT]; 
/// Handler Function
intr_handler idt_table[IDT_DESC_CNT];

/** General Handler **/
static void general_intr_handler(uint8_t vec_nr){
	if (vec_nr == 0x27 || 		//IRQ7 is spurious Int
		vec_nr == 0x2f )		//IRQ15	is reserved
	{ return;}
	put_str("int vector : 0x");
	put_int(vec_nr);
	put_char('\n');
}

/** Registrate **/
static void exception_init(void){
	for(int i=0; i<IDT_DESC_CNT; i++){
		idt_table[i] = &general_intr_handler;
		intr_name[i] = "Initialized but no value";
	}
	intr_name[0] = "#DE Divide Error";
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR BOUND Range Exceeded Exception";
	intr_name[6] = "#UD Invalid Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invalid TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	// intr_name[15] 第15项是intel保留项，未使用
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
}

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
	p_gdesc -> func_offset_low_word = (uint32_t)function & 0x0000FFFF;
	p_gdesc -> selector = SELECTOR_K_CODE;
	p_gdesc -> dcount = 0;
	p_gdesc -> attribute = attr;
	p_gdesc -> func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
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
	exception_init();
	pic_init();

	uint64_t idt_operand = ((sizeof(idt)-1) | ((uint64_t)(uint32_t)idt<<16));
	asm volatile ("lidt %0;"::"m"(idt_operand));
	put_str("idt_init done.\n");
}
