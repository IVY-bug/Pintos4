#include "vm/frame.h"
#include "vm/page.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

struct list_elem *cursor;
struct list frame_table;

void 
falloc_init (void)
{
	list_init(&frame_table);
}

void
falloc_set_frame (void *kaddr, struct spage_entry* spe)
{
	struct frame_entry *e = 
	(struct frame_entry *) malloc(sizeof(struct frame_entry));
	e->frame = kaddr;
	e->spe = spe;
	e->t = thread_current();

	list_push_back(&frame_table, &e->elem);
}

void
falloc_free_frame (void *kaddr)
{
	struct list_elem *e;
	
	for (e = list_begin(&frame_table);
		e != list_end(&frame_table); e = list_next(e))
    {
    	struct frame_entry *f = list_entry(e, struct frame_entry, elem);
    	if(f->frame == kaddr)
       	{
       		list_remove(e);
       		free(f);
       		break;
       	}
    }
}

struct frame_entry *
find_victim(void)
{
	cursor = list_begin(&frame_table);
	struct frame_entry *victim = NULL;
	
	while(true)
	{
		struct frame_entry *temp = list_entry(cursor, struct frame_entry, elem);

		if(pagedir_is_accessed(temp->t->pagedir, temp->frame))
		{
			pagedir_set_accessed(temp->t->pagedir, temp->frame, false);
			pagedir_set_accessed(temp->t->pagedir, temp->spe->uaddr, false);
		}
		else
		{
			victim = temp;
			cursor = list_next(cursor);
			if(cursor == list_end(&frame_table))
				cursor = list_begin(&frame_table);
			return victim;
		}

		cursor = list_next(cursor);
		if(cursor == list_end(&frame_table))
			cursor = list_begin(&frame_table);
	}
	return victim;
}
