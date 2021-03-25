#include "init.h"

#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "tss.h"
#include "console.h"
#include "keyboard.h"

void init_all(){
	idt_init();
	timer_init();
	mem_init();
	thread_init();
	console_init();
	keyboard_init();
	tss_init();
}
