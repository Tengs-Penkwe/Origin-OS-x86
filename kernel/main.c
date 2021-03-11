#include "print.h"
#include "init.h"

int main(void)
{
	init_all();
	asm volatile ("sti");
	while(1);
}
