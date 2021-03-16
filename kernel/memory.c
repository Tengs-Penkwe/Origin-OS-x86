#include "memory.h"
#include "stdint.h"
#include "global.h"

#include "bitmap.h"
#include "string.h"

#include "debug.h"
#include "print.h"

#define PG_SIZE 4096
//0xc009f000 is the stack top of kernel
//oxc009e000 is the PCB of main thread in kernel
#define MEM_BITMAP_BASE		0xc009a000
#define K_HEAP_START		0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

struct pool {
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
};

struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;

/** find pg_cnt pages in pool that pf indicates **/
///return start virtual address
///return NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0; 
	if (pf==PF_KERNEL){
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap,pg_cnt);
		if(bit_idx_start = -1){
			return NULL;
		}
		while(cnt < pg_cnt){
			bitmap_set(&kernel_vaddr.vaddr_bitmap,bit_idx_start + cnt++,1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	} else {
		//
	}
	return (void*)vaddr_start;
}

/** get the pte pointer correspond to vaddr **/
uint32_t* pte_ptr(uint32_t vaddr){

}

/** get the pde pointer correspond to vaddr **/
uint32_t* pde_ptr(uint32_t vaddr){

}

/** get 1 page in m_pool **/
///Return physical addr of page
///Return NULL
static void* palloc(struct pool* m_pool){
//! atom operation needed
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap,1);
	if (bit_idx == -1 ){
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap,bit_idx,1);
	uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
	return (void*)page_phyaddr;
}


/** Initialize Memory Pool **/
static void mem_pool_init(uint32_t all_mem){
	put_str("    mem_pool_init start\n");
	
	//Memory situation
	uint32_t page_table_size = PG_SIZE * (1 + 1 + 254);
	uint32_t used_mem = page_table_size + 0x100000;		//Low 1MiB
	uint32_t free_mem = all_mem - used_mem;

	//Memory -> Page
	uint16_t all_free_pages = free_mem / PG_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;

	//Length of Bitmap
	uint32_t kbm_length = kernel_free_pages / 8;
	uint32_t ubm_length = user_free_pages / 8;
	//Start Address of Memory Pool
	uint32_t kp_start = used_mem;
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	/* bitmap of memory pool and bitmap */
	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

	/* output memoey pool information */
	put_str("        kernel_pool.bitmap_start:");
	put_int((int)kernel_pool.pool_bitmap.bits);
	put_str("kernel_pool.phy_addr_start:");
	put_int((int)kernel_pool.phy_addr_start);
		put_str("\n");
		put_str("        user_pool.bitmap_start:");
		put_int((int)user_pool.pool_bitmap.bits);
		put_str("user_pool.phy_addr_start:");
		put_int((int)user_pool.phy_addr_start);
		put_str("\n");

	//set bitmap as 0
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	//Initialise the virtual address of kernel
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = \
					(void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);	
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	put_str("    mem_pool_init done\n");
}

void mem_init(){
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = 0x2000000; //(*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done\n");
}
