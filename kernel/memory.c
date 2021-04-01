#include "memory.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "stdio.h"

#include "bitmap.h"
#include "string.h"
#include "sync.h"
#include "thread.h"			//u_block_descs
#include "interrupt.h"		//enum intr_status

#define PG_SIZE 4096
//0xc009f000 is the stack top of kernel
//oxc009e000 is the PCB of main thread in kernel
#define MEM_BITMAP_BASE		0xc009a000
#define K_HEAP_START		0xc0100000

#define PDE_IDX(addr)	((addr & 0xFFC00000) >> 22)
#define PTE_IDX(addr)	((addr & 0x003FF000) >> 12)
struct pool {
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
	struct lock lock;			//! mutex when applying memory 
};
struct pool kernel_pool, user_pool;
struct virtual_addr kernel_vaddr;	//! Only one kernel space, one kernel virtual address

struct mem_block_desc k_block_descs[DESC_CNT];

static void page_table_add(void* _vaddr, void* _page_phyaddr);
static void* palloc(struct pool* m_pool);
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt);
static void mem_pool_init(uint32_t all_mem);

/** get 1 page in m_pool **/
	///Return physical addr of page
	///Return NULL
static void* palloc(struct pool* m_pool){
//! atom operation needed
	int bit_idx = -1;
	bit_idx = bitmap_scan(&m_pool->pool_bitmap,1);
	if (bit_idx == -1){
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	return (void*) (m_pool->phy_addr_start + bit_idx * PG_SIZE);
}

/** find pg_cnt pages in pool that pf indicates **/
	///return start virtual address
	///return NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0; 
	if (pf==PF_KERNEL) 		//! Kernel Memory Pool 
	{ 
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap,pg_cnt);
		if(bit_idx_start == -1){
			return NULL;
		}
		while(cnt < pg_cnt){
			bitmap_set(&kernel_vaddr.vaddr_bitmap,bit_idx_start + cnt++,1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + (bit_idx_start * PG_SIZE);
	}
	else 		//! User Memory Pool
	{
		struct task_struct* cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap,pg_cnt);
		if(bit_idx_start == -1){
			return NULL;
		}
		while(cnt < pg_cnt){
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap,bit_idx_start + cnt++,1);
		}
		vaddr_start = cur->userprog_vaddr.vaddr_start + (bit_idx_start * PG_SIZE);
	}
	return (void*)vaddr_start;
}

/** get the pte pointer correspond to vaddr **/
uint32_t* pte_ptr(uint32_t vaddr){
	uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}

/** get the pde pointer correspond to vaddr **/
uint32_t* pde_ptr(uint32_t vaddr){
	uint32_t* pde = (uint32_t*) (0xfffff000 + PDE_IDX(vaddr) * 4);
	return pde;
}

/** create mapping between _vaddr and _page_phyaddr **/
static void page_table_add(void* _vaddr, void* _page_phyaddr){
	uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);

	//verify if this page exists
	if (*pde & 0x00000001){
		ASSERT(!(*pte & 0x00000001));

		if(! (*pte & 0x00000001)){
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		} else {
			PANIC("PTE repeat");
			*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		}
	} else {
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool); 	//address to store PTE, not PDE
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);		//Store PTE address in PD
		
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		ASSERT(!(*pte & 0x00000001));
		
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}

/* Allocate pg_cnt pages */
	//return virtual address
	//return NULL
void * malloc_page (enum pool_flags pf, uint32_t pg_cnt){
	/***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上两步得到的虚拟地址和物理地址在页表中完成映射
	***************************************************************/
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);

	void* vaddr_start = vaddr_get(pf,pg_cnt);
	if (vaddr_start == NULL){
		return NULL;
	}

	uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool:&user_pool;

	while(cnt-- > 0){
		void* page_phyaddr = palloc(mem_pool);
		if (page_phyaddr == NULL){
			//
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

/** Apply 1 page in Kernel pool **/
	///Return virtual address
	///Return NULL
void* get_kernel_pages(uint32_t pg_cnt){
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL){
		memset(vaddr, 0, pg_cnt*PG_SIZE);
	}
	return vaddr;
}

void* get_user_pages(uint32_t pg_cnt){
	lock_acquire(&user_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	if (vaddr != NULL){
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	lock_release(&user_pool.lock);
	return vaddr;
}

void* get_a_page(enum pool_flags pf, uint32_t vaddr){
	/***********   get_a_page 是2个动作的合成:   ***********
      0 Already have applied vaddr, sent here
      1 通过palloc在物理内存池中申请物理页
      2 通过page_table_add将以上两步得到的虚拟地址和物理地址在页表中完成映射
	***************************************************************/
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	lock_acquire(&mem_pool->lock);

	struct task_struct* cur = running_thread();
	/* Set bitmap */
	int32_t bit_idx = -1;
	if (cur->pgdir != NULL && pf == PF_USER) {
		bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
		ASSERT (bit_idx > 0);
		bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
	}
	else if (cur->pgdir == NULL && pf == PF_KERNEL) {
		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		ASSERT (bit_idx > 0);
		bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
	} else {
		PANIC("get_a_page:not allowed kernel allocing userspace or user allocing kernelspace by get_a_page");
	}

	/* Get Physical Page */
	void* page_phyaddr = palloc(mem_pool);
	if (page_phyaddr == NULL) { return NULL; }
	page_table_add((void*)vaddr, page_phyaddr);

	lock_release(&mem_pool->lock);
	return (void*) vaddr;
}

uint32_t addr_v2p(uint32_t vaddr){
	uint32_t* pte = pte_ptr(vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
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

	/* output memory pool information */
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

	//Initialise lock
	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);

	//Initialise the virtual address of kernel
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	kernel_vaddr.vaddr_bitmap.bits = \
					(void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);	
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	put_str("    mem_pool_init done\n");
}

void block_desc_init(struct mem_block_desc* desc_array){
	uint16_t desc_idx, block_size = 16;
	for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++){
		desc_array[desc_idx].block_size = block_size;
		/* Number of mem_blocks in arena */
		desc_array[desc_idx].blocks_per_arena = \
			(PG_SIZE - sizeof(struct arena)) / block_size;
		list_init(&desc_array[desc_idx].free_list);
		block_size *= 2;		// Next Type of Memory Block
	}
}

//! Return the idx-th mem_block in arena
static struct mem_block* arena2block(struct arena* a, uint32_t idx){
	return (struct mem_block*) \
		((uint32_t)a + sizeof(struct arena) + idx * a->desc->block_size);
}

//! Return the arena address of  mem_block b
static struct arena* block2arena(struct mem_block* b){
	return (struct arena*)((uint32_t)b & 0xfffff000);
}

/* 在堆中申请size字节内存 */
void* sys_malloc(uint32_t size) {
    enum pool_flags PF;
    struct pool* mem_pool;
    uint32_t pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

    /* 判断用哪个内存池*/
    if (cur_thread->pgdir == NULL) {     // 若为内核线程
       	PF = PF_KERNEL; 
        pool_size = kernel_pool.pool_size;
        mem_pool = &kernel_pool;
        descs = k_block_descs;
    } else {				      // 用户进程pcb中的pgdir会在为其分配页表时创建
        PF = PF_USER;
        pool_size = user_pool.pool_size;
        mem_pool = &user_pool;
        descs = cur_thread->u_block_descs;
    }

    /* 若申请的内存不在内存池容量范围内则直接返回NULL */
    if (!(size > 0 && size < pool_size)) {
        return NULL;
    }
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);

    /* 超过最大内存块1024, 就分配页框 */
    if (size > 1024) {
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);    // 向上取整需要的页框数
		
        a = malloc_page(PF, page_cnt);

        if (a != NULL) {
            memset(a, 0, page_cnt * PG_SIZE);	 // 将分配的内存清0

            /* 对于分配的大块页框,将desc置为NULL, cnt置为页框数,large置为true */
            a->desc = NULL;
         	a->cnt = page_cnt;  
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1);		 // 跨过arena大小，把剩下的内存返回
        } else {
			lock_release(&mem_pool->lock); 
            return NULL;
        }
    } else {    // 若申请的内存小于等于1024,可在各种规格的mem_block_desc中去适配
        uint8_t desc_idx;

        /* 从内存块描述符中匹配合适的内存块规格 */
        for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
            if (size <= descs[desc_idx].block_size) {  // 从小往大后,找到后退出
                break;
            }
        }

        /* 若mem_block_desc的free_list中已经没有可用的mem_block,
         * 就创建新的arena提供mem_block */
        if (list_empty(&(descs[desc_idx].free_list))) {
            a = malloc_page(PF, 1);       // 分配1页框做为arena
            if (a == NULL) {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PG_SIZE);

            /* 对于分配的小块内存,将desc置为相应内存块描述符,
             * cnt置为此arena可用的内存块数,large置为false */
            a->desc = &descs[desc_idx];
            a->cnt = descs[desc_idx].blocks_per_arena;
            a->large = false;
            uint32_t block_idx;

            enum intr_status old_status = intr_disable();

            /* 开始将arena拆分成内存块,并添加到内存块描述符的free_list中 */
            for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
				b = arena2block(a, block_idx); 
                ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }

        /* 开始分配内存块 */
		b = elem2entry(struct mem_block, \
				free_elem, list_pop(&descs[desc_idx].free_list));
        memset(b, 0, descs[desc_idx].block_size);

        a = block2arena(b);  // 获取内存块b所在的arena
        a->cnt--;		   // 将此arena中的空闲内存块数减1
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}

/****** Memory Free *******/
void pfree(uint32_t page_phy_addr) {
	struct pool* mem_pool;
	uint32_t bit_idx = 0;
	if (page_phy_addr >= user_pool.phy_addr_start){
		mem_pool = &user_pool;
		bit_idx = (page_phy_addr - user_pool.phy_addr_start) / PG_SIZE;
	} else {
		mem_pool = &kernel_pool;
		bit_idx = (page_phy_addr - kernel_pool.phy_addr_start) / PG_SIZE;
	}
	bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);
}

static void page_table_pte_remove(uint32_t vaddr) {
	uint32_t* pte = pte_ptr(vaddr);
	*pte &= ~PG_P_1;
	asm volatile ("invlpg %0"::"m"(vaddr):"memory");
}

static void vaddr_remove (enum pool_flags pf, void* _vaddr, uint32_t pg_cnt){
	uint32_t bit_idx_start=0, vaddr=(uint32_t)_vaddr, cnt=0;

	if (pf==PF_KERNEL) {
		bit_idx_start = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		while (cnt < pg_cnt) {
			bitmap_set(&kernel_vaddr.vaddr_bitmap, \
					bit_idx_start+cnt++, 0); 
		}
	} else {
		struct task_struct* cur = running_thread();
		bit_idx_start = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
		while (cnt < pg_cnt){
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, \
					bit_idx_start + cnt++, 0);
		}
	}
}

void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt){
	uint32_t vaddr = (int32_t)_vaddr, page_cnt = 0;
	ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);

	uint32_t page_phy_addr = addr_v2p(vaddr);
	ASSERT ((page_phy_addr % PG_SIZE) == 0 && page_phy_addr >= 0x102000);

	if (page_phy_addr >= user_pool.phy_addr_start) {
		vaddr -= PG_SIZE;
		while(page_cnt < pg_cnt) {
			vaddr += PG_SIZE;
			page_phy_addr = addr_v2p(vaddr);
			ASSERT((page_phy_addr % PG_SIZE) == 0 && page_phy_addr >= user_pool.phy_addr_start);

			pfree(page_phy_addr);
			page_table_pte_remove(vaddr);
			page_cnt++;
		}
		vaddr_remove(pf, _vaddr, page_cnt);
	} else {
		vaddr -= PG_SIZE;
		while(page_cnt < pg_cnt){
			vaddr += PG_SIZE;
			page_phy_addr = addr_v2p(vaddr);
			ASSERT((page_phy_addr % PG_SIZE) == 0 && page_phy_addr >= kernel_pool.phy_addr_start && page_phy_addr < user_pool.phy_addr_start);
			pfree(page_phy_addr);
			page_table_pte_remove(vaddr);
			page_cnt++;
		}
		vaddr_remove(pf, _vaddr, page_cnt);
	}
}

void sys_free(void* ptr) {
	ASSERT (ptr != NULL);
	if (ptr!=NULL) {
		// Decide which pool to free 
		enum pool_flags PF;
		struct pool* mem_pool;
		if (running_thread()->pgdir == NULL) {
			ASSERT((uint32_t)ptr >= K_HEAP_START);
			PF = PF_KERNEL;
			mem_pool = &kernel_pool;
		} else {
			PF = PF_USER;
			mem_pool = &user_pool;
		}

		lock_acquire(&mem_pool->lock);
		//
		struct mem_block* b = ptr;
		struct arena* a = block2arena(b);
		ASSERT(a->large == 0 || a->large == 1);
		if (a->desc == NULL && a->large == true) { //bigger than 1024
			mfree_page(PF, a, a->cnt);
		} else {						//smaller than 1024
			list_append(&a->desc->free_list, &b->free_elem);

			//judge if the mem_block inside arena are free
			if (++a->cnt == a->desc->blocks_per_arena) {
				for (uint32_t block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
					struct mem_block* b = arena2block(a, block_idx);
					ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
					list_remove (&b->free_elem);
				}
				mfree_page(PF, a, 1);
			}
		}
		lock_release(&mem_pool->lock);
	}
}

void mem_init(){
	put_str("mem_init start\n");
	uint32_t mem_bytes_total = 0x2000000; //(*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	/* prepare for malloc */
	block_desc_init(k_block_descs);
	put_str("mem_init done\n");
}
