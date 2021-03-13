#include "init.h"
#include "interrupt.h"
#include "timer.h"

void init_all(){
	idt_init();
	timer_init();
}
