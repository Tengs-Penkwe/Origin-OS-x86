#include "print.h"
#include "init.h"
#include "interrupt.h"

void init_all(){
//	put_str("I'm initializing\n");
	idt_init();
	put_str("Initialization complete\n");
}
