#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "sync.h"
#include "debug.h"		//ASSERT

struct partition {
	char name[8];
	uint32_t start_lba;
	uint32_t sec_cnt;
	struct disk* my_disk;
	struct list_elem part_tag;
	struct super_block* sb;
	struct bitmap block_bitmap;
	struct bitmap inode_bitmap;
	struct list open_inodes;
};

struct disk {
	char name[8];
	struct ide_channel* my_channel;
	uint8_t dev_no;
	struct partition prim_parts[4];
	struct partition logic_parts[8];
};

struct ide_channel {
	char name[8];
	uint16_t port_base;		//start port number of this channel
	uint8_t irq_no;			//interrupt number
	bool expecting_intr;	//wait for interrupt from disk
	struct lock lock;
	struct semaphore disk_done;
	struct disk device[2];	
};

void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt); 
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt); 
void intr_hd_handler(uint8_t irq_no);
void ide_init(void);

#endif //__DEVICE_IDE_H
