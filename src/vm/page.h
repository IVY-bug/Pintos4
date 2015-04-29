#include "lib/kernel/list.h"
#include "threads/thread.h"

void spage_init(struct thread *);
void spage_insert(struct thread *, void *, void *, bool);
void spage_clear(struct thread *);
void spage_remove(struct thread *, void*);
