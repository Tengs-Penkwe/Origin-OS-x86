#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "list.h"

/** Generic Function Type, Often used as argument in thread function **/
typedef void thread_func(void*);

enum task_status{
	TASK_RUNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIED
};

/********** Stack When Interrupting **********
 * Save Environment when entering interruption
 *
 *********************************************/
struct intr_stack{
	uint32_t vec_no;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;		//Why?
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	//Done by cpu
	uint32_t err_code;
	void (*eip) (void);
	uint32_t cs;
	uint32_t eflags;
	void* esp;
	uint32_t ss;
};

/********** Stack for Thread **********
 * 
 *********************************************/
struct thread_stack{
	//Belongs to calling function 
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;

	//When thread executes at first timr, eip points to kernel_thread
	//Other time, it points to the return address of switch_to
	void (*eip) (thread_func* func, void* func_arg);

	/** Only used when its firstly scheduled to CPU **/
	void (*unused_retaddr);		//do nothing, just occupy the place
	thread_func* function;		//name of function that kernel_thread called
	void* func_arg;				//parameter needed by function that kernel_thread called
};

/** PCB of process or thread **/
struct task_struct{
	uint32_t* self_kstack;		//Each threads in kernel use their stack
	enum task_status status;
	char name[16];
	uint8_t priority;	
	uint8_t ticks;
	uint32_t elapsed_ticks;

	struct list_elem general_tag;
	struct list_elem all_list_tag;

	uint32_t* pgdir;		//Virtual address of PD

	uint32_t stack_magic;
};

struct task_struct* running_thread(void);
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);
void schedule(void);
void thread_init(void);
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);

#endif //__THREAD_THREAD_H
