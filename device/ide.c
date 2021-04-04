#include "ide.h"
#include "global.h"
#include "stdio.h"
#include "stdio-kernel.h"
#include "io.h"
#include "string.h"		//memset
#include "timer.h"		//mtime_sleep
#include "interrupt.h"	//register_handler

#define reg_data(channel)		(channel->port_base + 0)
#define reg_error(channel)		(channel->port_base + 1)
#define reg_sect_cnt(channel)	(channel->port_base + 2)
#define reg_lba_l(channel)		(channel->port_base + 3)
#define reg_lba_m(channel)		(channel->port_base + 4)
#define reg_lba_h(channel)		(channel->port_base + 5)
#define reg_dev(channel)		(channel->port_base + 6)
#define reg_status(channel)		(channel->port_base + 7)
#define reg_cmd(channel)		(reg_status(channel))
#define reg_alt_status(channel)	(channel->port_base + 0x206)
#define reg_ctl(channel)		reg_alt_status(channel)	

//! Status
#define BIT_STAT_BSY	 		0x80
#define BIT_STAT_DRDY			0x40
#define BIT_STAT_DRQ			0x08

//! Device
#define BIT_DEV_MBS				0xA0	//fixed
#define BIT_DEV_LBA				0x40
#define BIT_DEV_DEV				0x10

//! Command
#define CMD_IDENTIFY			0xec
#define CMD_READ_SECTOR			0x20     // 读扇区指令
#define CMD_WRITE_SECTOR		0x30	 // 写扇区指令

//! Maximum Write-Read Sectors
#define max_lba ((64*1024*1024/512) - 1)

static void select_disk(struct disk* hd);
static void select_sector (struct disk* hd, uint32_t lba, uint8_t sec_cnt) ;
static void cmd_out(struct ide_channel* channel, uint8_t cmd) ;
static void read_from_sector (struct disk* hd, void* buf, uint8_t sec_cnt) ;
static void write2sector (struct disk* hd, void* buf, uint8_t sec_cnt) ;
static bool busy_wait(struct disk* hd) ;

uint8_t channel_cnt;
struct ide_channel channels[2];

int32_t ext_lba_base = 0;			//!Extend Partition Table Start Sector
int8_t p_no = 0, l_no = 0;
struct list partition_list;

struct partition_table_entry {
	uint8_t bootable;
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_chs;
	uint8_t fs_type;
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_chs;
	uint32_t start_lba;
	uint32_t sec_cnt;
} __attribute__((packed));

struct boot_sector {
	uint8_t other[446];
	struct partition_table_entry partition_table[4];
	uint16_t signature;
} __attribute__((packed));

static void select_disk(struct disk* hd){
	uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
	if (hd->dev_no == 1) {			//! Slave Disk
		reg_device |= BIT_DEV_DEV;
	}
	outb(reg_dev(hd->my_channel), reg_device);
}

//! send to disk device start LBA address and sector number
static void select_sector (struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
	ASSERT(lba <= max_lba);
	struct ide_channel* channel = hd->my_channel;
	outb(reg_sect_cnt(channel), sec_cnt);
	outb(reg_lba_l(channel), lba);
	outb(reg_lba_m(channel), lba >> 8);
	outb(reg_lba_h(channel), lba >> 16);
	outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

//! send to channel: command
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
	channel->expecting_intr = true;
	outb(reg_cmd(channel), cmd);
}

//! Read sec_cnt sectors from disk to buf
static void read_from_sector (struct disk* hd, void* buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	insw(reg_data(hd->my_channel), buf, size_in_byte/2);
}

//! Write sec_cnt sectors from buf to disk
static void write2sector (struct disk* hd, void* buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	outsw(reg_data(hd->my_channel), buf, size_in_byte/2);
}

//! Wait for 30 seconds, 
static bool busy_wait(struct disk* hd) {
	struct ide_channel* channel = hd->my_channel;
	uint16_t time_limit = 30 * 1000;
	while (time_limit -=10 >= 0){
		if (!(inb(reg_status(channel)) & BIT_STAT_BSY)) {
			return (inb(reg_status(channel)) & BIT_STAT_DRQ);
		} else {
			mtime_sleep(10);
		}
	}
	return false;
}

/** Read sec_cnt sectors to buf **/
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba);		ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);
	//1. choose disk to operate
	select_disk(hd);

	uint32_t secs_op;		//sectors need to operate
	uint32_t secs_done = 0;		//sectors operated
	while(secs_done < sec_cnt){
		if((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}
		//2. write beginning sector and sector number
		select_sector(hd, lba + secs_done, secs_op);
		//3. write order into reg_cmd
		cmd_out(hd->my_channel, CMD_READ_SECTOR);
			/** Block itself 
			 * wake until disk sends interruption **/
			sema_down(&hd->my_channel->disk_done);
		//4. detect disk status
		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s read sector %d failed !\n", hd->name, lba);
			PANIC(error);
		}
		//5. read data
		read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
		secs_done += secs_op;
	}
	lock_release(&hd->my_channel->lock);
}

/** Write sec_cnt sectors to buf **/
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba);		ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);
	select_disk(hd);

	uint32_t secs_op;		//sectors need to operate
	uint32_t secs_done = 0;		//sectors operated
	while(secs_done < sec_cnt){
		if((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}
		select_sector(hd, lba + secs_done, secs_op);
		cmd_out(hd->my_channel, CMD_WRITE_SECTOR);
		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s read sector %d failed !\n", hd->name, lba);
			PANIC(error);
		}
		write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);
		sema_down(&hd->my_channel->disk_done);
		secs_done += secs_op;
	}
	lock_release(&hd->my_channel->lock);
}

//! swap len bytes and store it in buf
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
	uint8_t idx;
	for (idx = 0; idx < len; idx+= 2) {
		buf[idx + 1] = *dst++;
		buf[idx] = *dst++;
	}
	buf[idx] = '\0';
}

static void identify_disk(struct disk* hd) {
	//! We only do this when initialization. so there is no need to acquire lock or set expecting_intr
	char id_info[512];
	select_disk(hd);
	cmd_out(hd->my_channel, CMD_IDENTIFY);
	// Wait until interrupt
	sema_down(&hd->my_channel->disk_done);
	if (!busy_wait(hd)) {
		char error[64];
		sprintf(error, "%s identify failed!!!!!!\n", hd->name);
		PANIC(error);
	}
	read_from_sector(hd, id_info ,1 );
	//! byte order
	char buf[64];
	uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
	swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
	printk("   disk %s info:\n      SN: %s\n", hd->name, buf);
	memset(buf, 0, sizeof(buf));
	swap_pairs_bytes(&id_info[md_start], buf, md_len);
	printk("      MODULE: %s\n", buf);
	uint32_t sectors = *(uint32_t*)&id_info[60 * 2];
	printk("      SECTORS: %d\n", sectors);
	printk("      CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
}

static void partition_scan(struct disk* hd, uint32_t ext_lba) {
	struct boot_sector* bs = sys_malloc(sizeof(struct boot_sector));
	ide_read(hd, ext_lba, bs, 1);
	uint8_t part_idx = 0;
	struct partition_table_entry* p = bs->partition_table;

	while (part_idx++ < 4) {
		if (p->fs_type == 0x05) {
			if (ext_lba_base != 0) {
				partition_scan(hd, p->start_lba + ext_lba_base);
			} else {
				ext_lba_base = p->start_lba;
				partition_scan(hd, p->start_lba);
			}
		} else if (p->fs_type != 0) {
			if (ext_lba == 0) {
				hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
				hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
				hd->prim_parts[p_no].my_disk = hd;
				list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
				sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
				p_no ++;
				ASSERT(p_no < 4);
			} else {
				hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
				hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
				hd->logic_parts[l_no].my_disk = hd;
				list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
				sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5);
				l_no ++;
				if (l_no >= 8) 
					return;
			}
		}
		p++;
	}
	sys_free(bs);
}

static bool partition_info(struct list_elem* pelem, int arg UNUSED) {
	struct partition* part = elem2entry(struct partition, part_tag, pelem);
	printk("   %s start_lba:0x%x, sec_cnt:0x%x\n",part->name, part->start_lba, part->sec_cnt);
	
	return false;
}

void intr_hd_handler(uint8_t irq_no) {
	ASSERT(irq_no == 0x2e || irq_no == 0x2f);
	uint8_t ch_no = irq_no - 0x2e;
	struct ide_channel* channel = &channels[ch_no];
	ASSERT(channel->irq_no == irq_no);
	if (channel->expecting_intr) {
		channel->expecting_intr = false;
		sema_up(&channel->disk_done);
		inb(reg_status(channel));
	}
}

void ide_init(){
	printk("ide_init start\n");
	uint8_t hd_cnt = *((uint8_t*)(0x475));
	printk("   ide_init hd_cnt:%d\n",hd_cnt);
	list_init(&partition_list);
	ASSERT(hd_cnt >0 );
	channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

	struct ide_channel* channel;
	uint8_t channel_no = 0, dev_no = 0;

	while (channel_no < channel_cnt) {
		channel = &channels[channel_no];
		sprintf(channel->name, "IDE%d", channel_no);

		switch (channel_no) {
			case 0:
				channel->port_base = 0x01f0;
				channel->irq_no = 0x20 + 14;
				break;
			case 1:
				channel->port_base = 0x170;
				channel->irq_no = 0x20+15;
				break;
		}
		channel->expecting_intr = false;
		lock_init(&channel->lock);
		sema_init(&channel->disk_done, 0);
		register_handler(channel->irq_no, intr_hd_handler);

		/* 分别获取两个硬盘的参数及分区信息 */
		while (dev_no < 2) {
			struct disk* hd = &channel->device[dev_no];
			hd->my_channel = channel;
			hd->dev_no = dev_no;
			sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
			identify_disk(hd);	 // 获取硬盘参数
			if (dev_no != 0) {	 // 内核本身的裸硬盘(hd60M.img)不处理
				partition_scan(hd, 0);  // 扫描该硬盘上的分区
			}
			p_no = 0, l_no = 0;
			dev_no++;
		}
		dev_no = 0;
		channel_no++;
	}
	printk("\n   all partition info\n");
printk("\n  partition_list: 0x%x\n",&partition_list);
	/* 打印所有分区信息 */
	list_traversal(&partition_list, partition_info, (int)NULL);
	printk("ide_init done\n");
}
