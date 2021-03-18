#include "init.h"
#include "memory.h"

#include "debug.h"
#include "print.h"

int main(void)
{
	init_all();
	
	void* addr  = get_kernel_pages(3);
	put_str("\n");
	put_str("get kernel_free_page start vaddr is ");
	put_int((uint32_t)addr);
	put_str("\n");

	while(1);
	return 0;
}
