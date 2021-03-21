#include "sync.h"
#include "list.h"
#include "interrupt.h"

#include "global.h"
#include "debug.h"

void sema_init(struct semaphore* psema, uint8_t value){
	psema->value = value;
	list_init(&psema->waiters);
}

void lock_init(struct lock* plock){
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_init(&plock->semaphore,1);		//! Binary Semaphore
}

/** P(proberen) Operation for Semaphore **/
void sema_down (struct semaphore* psema){
	//! Close Int to ensure atomic operation
	enum intr_status old_status = intr_disable();
	
	while (psema->value == 0){			//! Hold by Others
		//! Make Sure Current Thread is not in the waiter list
		ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
		if (!elem_find(&psema->waiters, &running_thread()->general_tag)){
			PANIC("sema_down: thread blocked has been in waiter list\n");
		}
		
		//! If the semaphore==0, put itself into the waiter list, block itself
		list_append(&psema->waiters, &running_thread()->general_tag);
		thread_block(TASK_BLOCKED);
	}
	//! If semaphore==1 or unblocked, the thread get the lock
	psema->value--;		//! semaphore P
	ASSERT(psema->value == 0);
	//! Re-enable Int
	intr_set_status(old_status);
}

/** V(vorgohen) Operation for Semaphore **/
void sema_up (struct semaphore* psema){
	//! Close Int to ensure atomic operation
	enum intr_status old_status = intr_disable();

	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)){	//! Unblock the thread in waiters list
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;
	ASSERT(psema->value == 1);

	//! Re-enable Int
	intr_set_status(old_status);
}
