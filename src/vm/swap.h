#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "devices/disk.h"

void swap_init(void);
uint32_t swap_out(void*);
void swap_in(uint32_t, void*);
void * swap(void);

#endif
