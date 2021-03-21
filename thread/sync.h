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

#endif //__THREAD_SYNC_H
