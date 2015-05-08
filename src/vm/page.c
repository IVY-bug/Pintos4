#include "vm/page.h"
#include <list.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"

struct spage_entry *
spage_insert(struct thread *t, void *upage, struct file *f, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, bool load)
{
	struct spage_entry *spe =
	(struct spage_entry *) malloc(sizeof(struct spage_entry));
	spe->uaddr = upage;
	spe->f = f;
	spe->offset = ofs;
	spe->read_bytes = read_bytes;
	spe->zero_bytes = zero_bytes;
	spe->writable = writable;
	spe->indisk = false;
	spe->load = load;
	list_push_back(&t->spt, &spe->elem);
	return spe;
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

struct spage_entry* spage_find(struct thread* t, void *uaddr)
{
	struct list_elem *e;
	for (e = list_begin(&t->spt);
		e != list_end(&t->spt); e = list_next(e))
	{
		struct spage_entry *temp;
		temp = list_entry(e, struct spage_entry, elem);
		if(temp->uaddr == uaddr)
			return temp;
	}
	return NULL;
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
