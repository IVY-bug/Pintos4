#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/palloc.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"

struct frame_entry
{
	uintptr_t frame; // kernel virtual address of Frame
	void *kpage;
	struct thread *t;
	struct list_elem elem;
};

void falloc_init (void);
void *falloc_get_frame (enum palloc_flags);
void falloc_free_frame (void *);
struct frame_entry * find_victim(void);

#endif
