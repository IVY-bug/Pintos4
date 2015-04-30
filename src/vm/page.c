#include "vm/page.h"
#include <list.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"

struct spage_entry
{
	void *uaddr;
	void *kaddr;
	bool writable;
	bool swapped;
	struct list_elem elem;
};

struct spage_entry *
lookup_spage(struct thread *t, const void * vaddr)
{
	struct list_elem *e;
	for (e = list_begin(&t->spt);
		e != list_end(&t->spt); e = list_next(e))
	{
		struct spage_entry *temp;
		temp = list_entry(e, struct spage_entry, elem);
		if(temp->uaddr == vaddr)
		{
			return temp;
		}
	}
	return NULL;
}

void
spage_insert(struct thread *t, void *upage, void *kpage, bool writable)
{
	struct spage_entry *spe =
	(struct spage_entry *) malloc(sizeof(struct spage_entry));
	spe->uaddr = upage;
	spe->kaddr = kpage;
	spe->writable = writable;
	spe->swapped = false;
	list_push_back(&t->spt, &spe->elem);
}

void
spage_clear(struct thread *t)
{
	struct list_elem *e;
	for (e = list_begin(&t->spt);
		e != list_end(&t->spt); e = list_next(e))
	{
		list_remove(e);
		free(e);
	}
}

void
spage_remove(struct thread *t, void *uaddr)
{
	struct list_elem *e;
	for (e = list_begin(&t->spt);
		e != list_end(&t->spt); e = list_next(e))
	{
		struct spage_entry *temp;
		temp = list_entry(e, struct spage_entry, elem);
		if(temp->uaddr == uaddr)
		{
			list_remove(e);
			free(e);
		}
	}
}
