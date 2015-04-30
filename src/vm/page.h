#include "lib/kernel/list.h"
#include "threads/thread.h"

struct spage_entry * lookup_spage(struct thread *, const void *);
void spage_insert(struct thread *, void *, void *, bool);
void spage_clear(struct thread *);
void spage_remove(struct thread *, void*);
