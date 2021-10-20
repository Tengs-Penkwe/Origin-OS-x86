#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "global.h"		//bool
#include "list.h"		//list
#include "ide.h"		//partition

struct inode {
	uint32_t i_no;
	uint32_t i_size;	//size of files or sum of directory
	
	uint32_t i_open_cnts;
	bool write_deny;

	uint32_t i_sectors[13];
	struct list_elem inode_tag;
};

void inode_sync(struct partition* part, struct inode* inode, void* io_buf);
struct inode* inode_open(struct partition* part, uint32_t inode_no);
void inode_close(struct inode* inode);
void inode_init(uint32_t inode_no, struct inode* new_inode);
#endif //__FS_INODE_H
