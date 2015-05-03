#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "vm/page.h"

struct frame_entry
{
	void *frame; // frame의 virtual memory상의 주소
	struct spage_entry *spe; // 이 frame을 할당받고 있는 page에 대한 정보 
	struct thread *t; // 현재 이 frame을 점유하고 있는 thread
	struct list_elem elem;
};

void falloc_init (void);
void falloc_set_frame (void *, struct spage_entry*);
void falloc_free_frame (void *);
struct frame_entry * find_victim(void);

#endif
