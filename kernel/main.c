#include "print.h"

char* str = "Hallo Welt!";
int count = 0;

int main(void)
{
	put_char('k');
	put_str("I'm ready to go!");
//	put_int(2290618641);
	
//	asm("pusha;			\
		movl $4,%eax;	\
		movl $1,%ebx;	\
		movl str,%ecx;	\
		movl $11,%edx;	\
		int  $0x80;		\
		mov %eax,count;	\
		popa;			\
		");


	while(1);
}
