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
#include "syscall.h"
#include "syscall-init.h"
#include "stdio.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int prog_a_pid = 0, prog_b_pid = 0;
int test_var_a = 0, test_var_b = 0;

int main(void)
{
	init_all();
	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");

	intr_enable();
	printf(" Main PID:0x");
	console_put_int(getpid());
	console_put_char('\n');
	thread_start("consumer_a",31,k_thread_a," A_");
	thread_start("consumer_b",31,k_thread_b," B_");
	while(1);
	return 0;
}

void k_thread_a (void* arg){
	char* para = arg;
	printf(" From %s: Thread a PID:0x%x\n",arg, getpid());
	printf(" Process a PID:0x%x\n",prog_a_pid);
	while(1){
//		enum intr_status old_status = intr_disable();
//
//		if (!ioq_empty(&kbd_buf)){
//			printf(para);
//			char byte = ioq_getchar(&kbd_buf);
//			console_put_char(byte);
//		}
//		intr_set_status(old_status);
	}
}

void k_thread_b (void* arg){
	char* para = arg;
	printf(" From %s, Thread b PID:%d\n%c",arg, getpid(), '\n');
	printf(" Process b PID:%d\n",prog_b_pid);
	while(1){
	//	printf("v_b:0x");
	//	console_put_int(test_var_b);
//		enum intr_status old_status = intr_disable();
//
//		if (!ioq_empty(&kbd_buf)){
//			printf(para);
//			char byte = ioq_getchar(&kbd_buf);
//			console_put_char(byte);
//		}
//		intr_set_status(old_status);
	}
}

void u_prog_a(void){
	prog_a_pid = getpid();
	while(1){
	//	test_var_a++;
	}
}

void u_prog_b(void){
	prog_b_pid = getpid();
	while(1){
	//	test_var_b++;
	}
}
