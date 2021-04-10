#include "fs.h"
#include "ide.h"			//partition, disk, ide_channel 
#include "inode.h"			//inode
#include "super_block.h"	//super_block
#include "dir.h"			//dir_entry, dir
#include "stdio-kernel.h"	//printk
#include "string.h"			//memset, memcpy

extern struct ide_channel channels[2];

static void partition_format(struct partition* part) {
	/* blocks_bitmap_init */
	uint32_t boot_sector_sects = 1;
	uint32_t super_block_sects = 1;
	// Maximum file number is 4096
	uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
	uint32_t inode_table_sects = DIV_ROUND_UP(sizeof(struct inode) * MAX_FILES_PER_PART, SECTOR_SIZE);
	uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
	uint32_t free_sects = part->sec_cnt - used_sects;
	/******* how many sectors do bitmap of free sectors occupy? */
	uint32_t block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(free_sects, BITS_PER_SECTOR);
	uint32_t block_bitmap_bit_len = free_sects - block_bitmap_sects;
	block_bitmap_sects = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR);
	/********************************************************************/

	/** Initialize Super Block */
	struct super_block sb;
	sb.magic = 0x19590318;
	sb.sec_cnt = part -> sec_cnt;
	sb.inode_cnt = MAX_FILES_PER_PART;
	sb.part_lba_base = part -> start_lba;
	
	sb.block_bitmap_lba = sb.part_lba_base + 2;		//boot block and super_block
	sb.block_bitmap_sects = block_bitmap_sects;

	sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
	sb.inode_bitmap_sects = inode_bitmap_sects;

	sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
	sb.inode_table_sects = inode_table_sects;

	sb.data_start_lba = sb.inode_table_lba + sb.inode_table_sects + sb.inode_table_sects;
	sb.root_inode_no = 0;	
	sb.dir_entry_size = sizeof(struct dir_entry);

	printk("%s info:\n", part->name);
	printk("   magic:0x%x\n   part_lba_base:0x%x\n   all_sectors:0x%x\n   inode_cnt:0x%x\n   block_bitmap_lba:0x%x\n   block_bitmap_sectors:0x%x\n   inode_bitmap_lba:0x%x\n   inode_bitmap_sectors:0x%x\n   inode_table_lba:0x%x\n   inode_table_sectors:0x%x\n   data_start_lba:0x%x\n", sb.magic, sb.part_lba_base, sb.sec_cnt, sb.inode_cnt, sb.block_bitmap_lba, sb.block_bitmap_sects, sb.inode_bitmap_lba, sb.inode_bitmap_sects, sb.inode_table_lba, sb.inode_table_sects, sb.data_start_lba);

	//1.1 write spuer block into the 1st sector in this partition
	struct disk* hd = part -> my_disk;
	ide_write(hd, part->start_lba + 1, &sb, 1);
	printk("   super_block_lba:0x%x\n", part->start_lba + 1);
	// find the meta-information with buggest size, use it size as buffer
	uint32_t buf_size = (sb.block_bitmap_sects >= sb.inode_bitmap_sects ? sb.block_bitmap_sects : sb.inode_bitmap_sects);
	buf_size = (buf_size >= sb.inode_table_sects ? buf_size : sb.inode_table_sects) * SECTOR_SIZE;
	uint8_t* buf = (uint8_t*)sys_malloc(buf_size);	// 申请的内存由内存管理系统清0后返回

	//2 Initialize block_bitmap and write into sb.block_bitmap_lba
	buf[0] |= 0x01;		//reserve to root directory
	uint32_t block_bitmap_last_byte = block_bitmap_bit_len / 8;
	uint8_t block_bitmap_last_bit = block_bitmap_bit_len % 8;
	uint32_t last_size = SECTOR_SIZE - (block_bitmap_last_byte % SECTOR_SIZE);
	//2.1 set sectors
	memset(&buf[block_bitmap_last_byte], 0xff, last_size);
	//2.2 reset bits 
	uint8_t bit_idx = 0;
	while (bit_idx < block_bitmap_last_bit) {
		buf[block_bitmap_last_byte] &= ~(1 << bit_idx++);
	}
	ide_write(hd, sb.block_bitmap_lba, buf, sb.block_bitmap_sects);

	//3 Initialize inode block and write it into sb.inode_bitmap_lba
	memset(buf, 0, buf_size);
	buf[0] |= 0x1;		//root directory
	/* inode_bitmap has no no effective bit */
	ide_write(hd, sb.inode_bitmap_lba, buf, sb.inode_bitmap_sects);

	//4 Initialize inode array and write it into sb.inode_bitmap_lba
	memset(buf, 0, buf_size);
	struct inode* i = (struct inode*) buf;
	i -> i_size = sb.dir_entry_size * 2;		// . and ..
	i -> i_no = 0;								// root directory use the 0th inode
	i -> i_sectors[0] = sb.data_start_lba;		// memset
	
	ide_write(hd, sb.inode_table_lba, buf, sb.inode_table_sects);

	//5 Write root directory into sb.data_start_lba
	memset(buf, 0, buf_size);
	struct dir_entry* p_de = (struct dir_entry*)buf;
	
	//5.1 Initialize current directory
	memcpy(p_de->filename, ".", 1);
	p_de -> i_no = 0;
	p_de -> f_type = FT_DIRECTORY;
	p_de ++;

	//5.1 Initialize parent directory
	memcpy(p_de->filename, "..", 2);
	p_de -> i_no = 0;
	p_de -> f_type = FT_DIRECTORY;
	p_de ++;
	
	ide_write(hd, sb.data_start_lba, buf, 1);

	printk("   root_dir_lba:0x%x\n", sb.data_start_lba);
	printk("%s format done\n", part->name);
	sys_free(buf);
}

/** search file system in the disk, if none, create it **/
void filesys_init() {
	uint8_t channel_no = 0, dev_no = 0, part_idx = 0;

	//store super_block read from disk
	struct super_block* sb_buf = (struct super_block*) sys_malloc(sizeof(struct super_block));	//512 SECTOR_SIZE
	if (sb_buf == NULL) {
		PANIC("alloc memory failed!");
	}
	printk("searching filesystem......\n");
	while (channel_no < 2) {
		dev_no = 0;
		while (dev_no < 2) {
			if (dev_no == 0) {	//raw disk hd64Mi.img
				dev_no ++;
				continue;
			}
			struct disk* hd = &channels[channel_no].devices[dev_no];
			struct partition* part = hd->prim_parts;
			while (part_idx < 12) {
				if (part_idx == 4) {
					part = hd->logic_parts;
				}
				if (part->sec_cnt != 0) {
					memset(sb_buf, 0, SECTOR_SIZE);
					ide_read(hd, part->start_lba + 1, sb_buf, 1);
					if (sb_buf->magic == 0x19590318) {
						printk("%s has filesystem\n", part->name);
					} else {
						printk("formatting %s`s partition %s......\n", hd->name, part->name);
                        partition_format(part);
					}
				}
				part_idx++;
				part++;
			}
			dev_no++;
		}
		channel_no++;
	}
	sys_free(sb_buf);
}
