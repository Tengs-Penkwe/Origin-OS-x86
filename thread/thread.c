#include "thread.h"
#include "debug.h"
#include "print.h"

#include "string.h"
#include "memory.h"
#include "interrupt.h"
#include "process.h"

struct task_struct* main_thread;		//PCB of main thread
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag;	//Save the thread tag in list

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/** Get the PCB pointer of Currently running process **/
struct task_struct* running_thread(){
	uint32_t esp;
	asm volatile ("mov %%esp, %0":"=g"(esp));
	//Modulo to get the start address of PCB
	return (struct task_struct*)(esp & 0xfffff000);
}

/** Execute function(func_arg) by kernel_thread **/
static void kernel_thread(thread_func* function, void* func_arg){
	intr_enable();			//! Need to enable interrupt for thread at first time
	function(func_arg);
}

/** Initialize basic information of thread **/
void init_thread(struct task_struct* pthread, char* name, int prio){
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);
	//Encapslate main as a thread
	if (pthread == main_thread){
		pthread->status = TASK_RUNING; 
	} else {
		pthread->status = TASK_READY;
	}
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->priority = prio;
	pthread->ticks = prio;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	pthread->stack_magic = 0x19870916;
}

/** Initialize thread stack, put function and arguments inti thread_stack **/
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg){
	//stack for interruption
	pthread->self_kstack -= sizeof(struct intr_stack);
	//stack for thread
	pthread->self_kstack -= sizeof(struct thread_stack);
	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;

	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;
	kthread_stack->ebp = kthread_stack->ebx = \
	kthread_stack->edi = kthread_stack->esi = 0;
}

/** create thread **/
struct task_struct* thread_start(char* name,			\
								int prio,				\
								thread_func function,	\
								void* func_arg){
	//put PCB in kernel space
	struct task_struct* thread = get_kernel_pages(1);

	init_thread(thread, name, prio);
	thread_create(thread,function,func_arg);

	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	list_append(&thread_ready_list, &thread->general_tag);

	ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
	list_append(&thread_all_list, &thread->all_list_tag);

	return thread;
}

static void make_main_thread(void){
	main_thread = running_thread();
	init_thread(main_thread, "main",31);

	//Main thread is the current thread, so we only need to put it to the all list
	ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag);
}

/** Thread Scheduling **/
void schedule(){
	ASSERT(intr_get_status() == INTR_OFF);

	struct task_struct* cur = running_thread();
put_str("I'm here:");
put_int((uint32_t)cur);
put_char('\n');
	if(cur->status == TASK_RUNING){
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	} else {
		//
	}

	ASSERT(!list_empty(&thread_ready_list));
	thread_tag = NULL;
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct,	\
					general_tag, thread_tag);
	next->status = TASK_RUNING;

	/* Refresh Page Table */
	process_activate(next);
	switch_to(cur,next);
}

/** We use binary semaphore to solve race condition **/
void thread_block(enum task_status stat){
	//! stat should be TASK_BLOCKED, TASK_WAITING, TASK_HANGING to be not scheduled
	ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING));
	enum intr_status old_status = intr_disable();

	struct task_struct* cur_thread = running_thread();
	cur_thread -> status = stat;
	schedule();

	intr_set_status(old_status);		//! wait until thread is unblocked
}

void thread_unblock(struct task_struct* pthread){
	enum intr_status old_status = intr_disable();

	ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));
	if (pthread->status != TASK_READY){
		ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
		if(elem_find(&thread_ready_list, &pthread->general_tag)){
			PANIC("Thread Unblock: Blocked thread un ready list\n");
		}
		//! put thread in the begin of ready list 
		list_push(&thread_ready_list, &pthread->general_tag);
		pthread->status = TASK_READY;
	}
	intr_set_status(old_status);
}

void thread_init(void){
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	make_main_thread();
	put_str("thread_init done\n");
}
