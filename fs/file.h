#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "ide.h"
#include "dir.h"
#include "global.h"

#define MAX_FILE_OPEN 32

struct file {
	uint32_t fd_pos;	//! Maximum = sizeof(file)-1
	uint32_t fd_flags;
	struct inode* fd_inode;
};

enum std_fd {
	stdin_no,
	stdout_no,
	stderr_no
};

enum bitmap_type {
	INODE_BITMAP,
	BLOCK_BITMAP
};

#endif //__FS_FILE_H
