#include "vm/swap.h"
#include <list.h>

struct swap_entry
{
	
};

struct list swap_table;

disk_sector_t
find_emptyslot(void)
{
	uint32_t sec_no;
	return sec_no;
}

void
swap_out()
{
	disk_write (struct disk *d, disk_sector_t sec_no, const void *buffer);
}