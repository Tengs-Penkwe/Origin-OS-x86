#include "bitmap.h"

#include "stdint.h"
#include "string.h"
#include "interrupt.h"
#include "debug.h"

#include "print.h"

void bitmap_init(struct bitmap* btmp){
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_index){
	uint32_t byte_index = bit_index / 8;
	uint32_t bit_odd	= bit_index % 8;
	return (btmp->bits[byte_index] & (BITMAP_MASK << bit_odd));
}

/** Require cnt in bitmap **/
/// Return the index of post-suffix
/// Return -1 if failed
int bitmap_scan(struct bitmap* btmp, uint32_t cnt){
	uint32_t idx_byte = 0;
	while((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)){
		idx_byte++;
	}
	ASSERT(idx_byte <  btmp->btmp_bytes_len);
	if(idx_byte == btmp->btmp_bytes_len){
		return -1;
	}
	
	int idx_bit = 0;
	while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]){
		idx_bit++;
	}
	uint32_t bit_idx_start = idx_byte*8 + idx_bit;
	if(cnt==1) { return bit_idx_start; }

	//Note we have how many bit to judge
	uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start;
	//Record free bits' number we found
	uint32_t count = 1;
	uint32_t next_bit = bit_idx_start + 1;

	bit_idx_start = -1;
	while (bit_left-- > 0){
		if (!bitmap_scan_test(btmp,next_bit)){
			count++;
		}else{
			count = 0;
		}
		if(count == cnt){
			bit_idx_start = next_bit - cnt + 1;	//start itself is a bit
			break;
		}
		next_bit ++;
	}
	return bit_idx_start;
}

void bitmap_set(struct bitmap* btmp, uint32_t bit_index, int8_t value){
	ASSERT((value==0)||(value==1));
	uint32_t byte_index = bit_index / 8;
	uint32_t bit_odd	= bit_index % 8;

	if (value){
		btmp->bits[byte_index] |= (BITMAP_MASK << bit_odd);
	}else{
		btmp->bits[byte_index] &= ~(BITMAP_MASK << bit_odd);
	}
}
