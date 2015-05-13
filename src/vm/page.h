#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "lib/kernel/list.h"
#include "threads/thread.h"
#include "filesys/file.h"

struct spage_entry
{
	void *uaddr;
	struct file *f;
	off_t offset;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
	bool indisk;
	bool load;
	int mmap;
	uint32_t sec_no;
	struct list_elem elem;
};

struct spage_entry* spage_insert(struct thread *, void *, struct file *,
	off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, bool load, int mmap);
void spage_clear(struct thread *);
void spage_remove(struct thread *, void *);
struct spage_entry* spage_find(struct thread*, void *);
void spage_remove_mmap(struct thread *t, int mapping);

#endif
