#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "lib/kernel/list.h"
#include "threads/thread.h"

struct spage_entry
{
	void *uaddr;
	void *kaddr;
	bool writable;
	bool indisk;
	uint32_t sec_no;
	struct list_elem elem;
};

struct spage_entry * lookup_spage_by_kaddr(struct thread *, const void *);
struct spage_entry * lookup_spage_by_uaddr(struct thread *, const void *);
void spage_insert(struct thread *, void *, void *, bool);
void spage_clear(struct thread *);
void spage_remove(struct thread *, void*);

#endif
