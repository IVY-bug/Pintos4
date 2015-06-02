#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "lib/kernel/bitmap.h"


#define SECTOR_PER_PAGE (PGSIZE/ DISK_SECTOR_SIZE)


struct disk* swap_disk;
struct bitmap* swap_bitmap;

void swap_init(void)
{
	swap_disk = disk_get(1,1);
	swap_bitmap = bitmap_create(disk_size(swap_disk)/SECTOR_PER_PAGE);
	
	bitmap_set_all(swap_bitmap, 0);
}

uint32_t swap_out(void *frame)
{
	uint32_t index = bitmap_scan_and_flip(swap_bitmap, 0, 1, 0);

	int i;
	for(i=0;i<SECTOR_PER_PAGE;i++)
		disk_write(swap_disk, (index * SECTOR_PER_PAGE) + i, frame + (i * DISK_SECTOR_SIZE));

	return index;
}
void swap_in(uint32_t index, void *frame)
{
	bitmap_flip(swap_bitmap, index);
	
	int i;
	for(i=0;i<SECTOR_PER_PAGE;i++)
		disk_read(swap_disk, (index * SECTOR_PER_PAGE) + i, frame + (i * DISK_SECTOR_SIZE));

}

void *
swap (void)
{
  struct frame_entry *victim_frame = find_victim();
  
  struct spage_entry *victim_spage_entry = victim_frame->spe;
  uint32_t sectornum = swap_out(victim_frame->frame);

  pagedir_clear_page(victim_frame->t->pagedir,victim_spage_entry->uaddr);
  
  victim_spage_entry->indisk = true;
  victim_spage_entry->sec_no = sectornum;
  
  palloc_free_page(victim_frame->frame);
  falloc_free_frame(victim_frame->frame);

  void *kpage = palloc_get_page(PAL_USER|PAL_ZERO);
  ASSERT(kpage != NULL);
  
  return kpage;
}
