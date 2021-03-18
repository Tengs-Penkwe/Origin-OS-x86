#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

#define	PG_P_1		0b001
#define	PG_P_0		0b000
#define	PG_RW_R		0b000
#define	PG_RW_W		0b010
#define	PG_US_S		0b000
#define	PG_US_U		0b100

extern struct pool kernel_pool, user_pool;

enum pool_flags{
	PF_KERNEL = 1,
	PF_USER	  = 2
};

struct virtual_addr {
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

void* malloc_page(enum pool_flags, uint32_t pg_cnt);
void* get_kernel_pages(uint32_t pg_cnt);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
void mem_init(void);

#endif  //__KERNEL_MEMORY_H
