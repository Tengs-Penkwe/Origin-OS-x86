#include "init.h"

#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "tss.h"
#include "console.h"
#include "keyboard.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

void init_all(){
	idt_init();
	timer_init();
	mem_init();
	thread_init();
	console_init();
	keyboard_init();
	tss_init();
	syscall_init();
		intr_enable();
	ide_init();
	filesys_init();
}
