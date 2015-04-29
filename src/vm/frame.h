#include "threads/palloc.h"

void falloc_init (void);
void *falloc_get_frame (enum palloc_flags);
void falloc_free_frame (void *);
