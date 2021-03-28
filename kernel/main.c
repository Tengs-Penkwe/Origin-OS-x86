#include "debug.h"
#include "print.h"

#include "init.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"
#include "process.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int test_var_a = 0, test_var_b = 0;

int main(void)
{
	init_all();
	thread_start("consumer_a",31,k_thread_a," A_");
	thread_start("consumer_b",31,k_thread_b," B_");
	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");

	intr_enable();
	while(1);
	return 0;
}

void k_thread_a (void* arg){
	char* para = arg;
	while(1){
		console_put_str("v_a:0x");
		console_put_int(test_var_a);
//		enum intr_status old_status = intr_disable();
//
//		if (!ioq_empty(&kbd_buf)){
//			console_put_str(para);
//			char byte = ioq_getchar(&kbd_buf);
//			console_put_char(byte);
//		}
//		intr_set_status(old_status);
	}
}

void k_thread_b (void* arg){
	char* para = arg;
	while(1){
		console_put_str("v_b:0x");
		console_put_int(test_var_b);
//		enum intr_status old_status = intr_disable();
//
//		if (!ioq_empty(&kbd_buf)){
//			console_put_str(para);
//			char byte = ioq_getchar(&kbd_buf);
//			console_put_char(byte);
//		}
//		intr_set_status(old_status);
	}
}

void u_prog_a(void){
	while(1){
		test_var_a++;
	}
}

void u_prog_b(void){
	while(1){
		test_var_b++;
	}
}
