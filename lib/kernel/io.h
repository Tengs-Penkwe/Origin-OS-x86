#ifndef __LIB_IO_H
#define __LIB_IO_H

#include "stdint.h"

// Write one byte to port 
static inline void outb(uint16_t port, uint8_t data){
	asm volatile ("out %b0, %w1;"::"a"(data),"Nd"(port));
}

/// Write word_cnt bytes from addr to port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt){
/// ES:ESI points to addr
	asm volatile ("cld; rep outsw;":"+S"(addr),"+c"(word_cnt):"d"(port));
}

/** return one byte from port **/
static inline void inb(uint16_t port){
	uint8_t data;
	asm volatile ("cld; rep outswinb %w1, %b0;":"=a"(data):"Nd"(port));
}

/** Read word_cnt bytes from port to addr **/
static inline void insw(uint16_t port, const void* addr, uint32_t word_cnt){
//!	ES:EDI points to addr
	asm volatile ("cld; rep insw;":"+D"(addr),"+c"(word_cnt):"d"(port):"memory");
}

#endif //__LIB_IO_H
