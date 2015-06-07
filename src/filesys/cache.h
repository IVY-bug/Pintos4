#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/disk.h"
#include "filesys/off_t.h"

struct cache_elem
{
  int sec_no;
  uint8_t data[DISK_SECTOR_SIZE];
  bool dirty;
  bool accessed;
};

struct cache_elem cache[64];

void cache_init(void);
int cache_lookup(int);
int cache_victim(void);
void cache_read(int, void *, int, int);
void cache_write(int, const void *, int, int);
void cache_done(void);

#endif
