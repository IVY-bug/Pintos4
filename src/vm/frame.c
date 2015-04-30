#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

struct frame_entry
{
	uintptr_t frame; // kernel virtual address of Frame
	void *page;
	struct thread *t;
	struct list_elem elem;
};

struct list_elem *cursor;
struct list frame_table;
struct lock frame_lock;

void
falloc_init (void)
{
	lock_init(&frame_lock);
	list_init(&frame_table);
	cursor = list_begin(&frame_table);
}

void *
falloc_get_frame (enum palloc_flags flags)
{
	void *vaddr = palloc_get_page(flags);
	if(vaddr == NULL) //need swapping
	{
		return NULL;
	}

	uintptr_t paddr = vtop(vaddr);

	lock_acquire(&frame_lock);
	struct frame_entry *e = 
	(struct frame_entry *) malloc(sizeof(struct frame_entry));
	e->frame = paddr;
	e->page = vaddr;
	e->t = thread_current();
	list_push_back(&frame_table, &e->elem);
	lock_release(&frame_lock);

	return vaddr;
}

void
falloc_free_frame (void *page_addr)
{
	struct list_elem *e;
	struct frame_entry *f = NULL;

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

struct frame_entry *
find_victim(void)
{
	struct frame_entry *victim;

	while(true)
	{
		struct frame_entry *temp = list_entry(cursor, struct frame_entry, elem);
		
		if(pagedir_is_accessed(temp->t->pagedir, &temp->frame))
		{
			pagedir_set_accessed(temp->t->pagedir, &temp->frame, false);
			pagedir_set_accessed(temp->t->pagedir, temp->page, false);
		}
		else
		{
			victim = temp;
			cursor = list_next(cursor);
			if(cursor == list_end(&frame_table))
				cursor = list_begin(&frame_table);
			break;
		}

		cursor = list_next(cursor);
		if(cursor == list_end(&frame_table))
			cursor = list_begin(&frame_table);
	}

	return victim;
}
