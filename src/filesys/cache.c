#include "devices/disk.h"
#include "filesys/filesys.h"
#include "filesys/cache.h"
#include "lib/string.h"

int cursor;

void
cache_init(void)
{
  int i;
  cursor = 0;

  for(i = 0; i < 64; i++)
  {
    cache[i].dirty = false;
    cache[i].accessed = false;
    cache[i].sec_no = -1;
  }
}

int
cache_lookup(int sec_no)
{
	int i;
	for(i = 0; i < 64; i++)
	{
		if(cache[i].sec_no == sec_no)
			return i;
	}
	return -1;
}

int
cache_victim(void)
{
	int i;
	for(i = 0; i < 64; i++)
	{
		if(cache[i].sec_no == -1)
			return i;
	}
	
	int ret = cursor;
	if(cache[cursor].dirty)
		disk_write(filesys_disk, cache[cursor].sec_no, cache[cursor].data);
	if(cursor == 63)
		cursor = -1;
	cursor = cursor + 1;

	return ret;
}

void
cache_read(int sec_no, void *buffer, off_t size, off_t offset)
{
	int s = cache_lookup(sec_no);
	if(s == -1)
	{
		s = cache_victim();
		disk_read(filesys_disk, sec_no, cache[s].data);
	}
	memcpy(buffer, cache[s].data + offset, size);
	cache[s].sec_no = sec_no;
	cache[s].accessed = true;
}

void
cache_write(int sec_no, const void *buffer, off_t size, off_t offset)
{
	int s = cache_lookup(sec_no);
	if(s == -1)
	{
		s = cache_victim();
		disk_read(filesys_disk, sec_no, cache[s].data);
	}
	memcpy(cache[s].data + offset, buffer, size);
	cache[s].sec_no = sec_no;
	cache[s].accessed = true;
	cache[s].dirty = true;
}

void
cache_done(void)
{
	int i;
	for(i = 0; i < 64; i++)
	{
		if(cache[i].dirty)
			disk_write(filesys_disk, cache[i].sec_no, cache[i].data);
	}
}
