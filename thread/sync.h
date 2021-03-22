#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "thread.h"

/** The Struture of Semaphore **/
struct semaphore {
	uint8_t value;
	struct list waiters;
};

struct lock {
	struct task_struct* holder;		//! Holder of the lock
	struct semaphore semaphore;		//! Use binary seamphore to implement lock
	uint32_t holder_repeat_nr;		//! Times that holder applies lock
};

void sema_init(struct semaphore* psema, uint8_t value);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_init(struct lock* plock);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif //__THREAD_SYNC_H
