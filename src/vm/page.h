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
	uint32_t sec_no;
	struct list_elem elem;
};

struct spage_entry* spage_insert(struct thread *, void *, struct file *,
	off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, bool load);
void spage_clear(struct thread *);
void spage_remove(struct thread *, void *);
struct spage_entry* spage_find(struct thread*, void *);

#endif
