#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

enum pool_flags{
	PF_KERNEL = 1,
	PF_USER	  = 2
};

struct virtual_addr {
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

void mem_init(void);

//extern struct pool kernel_pool, user_pool;

#define	PG_P_1		0b001
#define	PG_P_0		0b000
#define	PG_RW_R		0b000
#define	PG_RW_W		0b010
#define	PG_US_S		0b000
#define	PG_US_U		0b100

#endif  //__KERNEL_MEMORY_H
