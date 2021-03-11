#include "print.h"
#include "init.h"
#include "interrupt.h"

void init_all(){
	idt_init();
	put_str("Initialization complete\n");
}
