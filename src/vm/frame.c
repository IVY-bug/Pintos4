#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

struct frame_entry
{
	uintptr_t frame; // Phisycal Address of Frame
	void *page;
	struct list_elem elem;
};

struct list frame_table;
struct lock frame_lock;

void
falloc_init (void)
{
	lock_init(&frame_lock);
	list_init(&frame_table);
}

void *
falloc_get_frame (enum palloc_flags flags)
{
	void *vaddr = palloc_get_page(flags);
	if(vaddr == NULL)
		return NULL;

	uintptr_t paddr = vtop(vaddr);

	lock_acquire(&frame_lock);
	struct frame_entry *e = 
	(struct frame_entry *) malloc(sizeof(struct frame_entry));
	e->frame = paddr;
	e->page = vaddr;
	list_push_back(&frame_table, &e->elem);
	lock_release(&frame_lock);

	return vaddr;
}

void
falloc_free_frame (void *page_addr)
{
	struct list_elem *e;
	struct frame_entry *f;

	for (e = list_begin(&frame_table);
		e != list_end(&frame_table); e = list_next(e))
    {
    	f = list_entry(e, struct frame_entry, elem);
       	if(f->frame == vtop(page_addr))
       	{
       		palloc_free_page(page_addr);

       		lock_acquire(&frame_lock);
       		list_remove(e);
       		lock_release(&frame_lock);
       		break;
       	}
    }
    free(f);
}
