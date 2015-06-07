#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    //disk_sector_t start;                /* First data sector. */
    off_t length;                       /* File size in bytes. */
    disk_sector_t direct[123];
    disk_sector_t indirect;
    disk_sector_t doubly_indirect;
    bool isdir;
    unsigned magic;                     /* Magic number. */
    //uint32_t unused[125];               /* Not used. */

  };

struct sector_table
{
  disk_sector_t table[128];
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

int
inode_sector(struct inode *inode)
{
  return (int) inode->sector;
}
bool
inode_isdir(struct inode *inode)
{
  return inode->data.isdir;
}
/* allocate_sectors
inode와 sectors를 받아서
어떤 한 파일이 sectors개수만큼 disk에서 할당받도록 해준다
두가지 경우가 있는데
아예 처음부터 새로 할당을 시작하는 경우와
원래 가지고 있던 부분에서 추가해서 할당하는 경우

*/
static bool
allocate_sectors(struct inode_disk *disk_inode, int sector , int cnt)
{
 // printf("@@@@@@@@@@@@ allocate_sectors start inode_length : %d sector : %d, cnt : %d @@@@@@@@@@@\n",disk_inode->length,sector,cnt);
 
  /* doubly indirect */
  int i, j;
  static char zeros[512];
  if(sector == 0) return true;
  struct sector_table *doubly_indirect_table;
  doubly_indirect_table = calloc(1, sizeof(struct sector_table));
  //printf("wwwww\n");
  if(cnt == 0)
  {
    //printf("@@@@@@@@@ make doubly_indrect_table @@@@@@@\n");
    if(!free_map_allocate(1, &disk_inode->doubly_indirect))
    {
      //printf("kakakakakkakakakakakakkakakakakak\n\n");
      return false;
    }
  }
  //printf("@@@@@@@@ doubly_indirect_table sector : %d @@@@@@@@@@@\n",disk_inode->doubly_indirect);
  cache_read(disk_inode->doubly_indirect, doubly_indirect_table, 512, 0);
    
  for(i = 0; i < 128 && sector > 0; i++)
  {
    struct sector_table *indirect_table = calloc(1, sizeof(struct sector_table));

    if(cnt == 0)
    {
      //printf("@@@@@@@ make indirect_table @@@@@@@@\n");
      if(!free_map_allocate(1, &doubly_indirect_table->table[i]))
        return false;
    }
    //printf("@@@@@@@@ indirect_table sector : %d @@@@@@@@@@@\n",doubly_indirect_table->table[i]);
  
    cache_read(doubly_indirect_table->table[i], indirect_table, 512, 0);
    
    for(j = 0; j < 128 && sector > 0; j++)
    {
      if(cnt == 0)
      {
        if(free_map_allocate(1, &indirect_table->table[j]))
        {
          //printf("@@@@@@@ add new sector to sector : %d  !!!!!@@@@@@@@@\n",indirect_table->table[j]);
          cache_write(indirect_table->table[j], zeros, 512, 0);
        }
        else
        {
          free(indirect_table);
          free(disk_inode);
          return false;
        }       
      }
      else
        cnt--;
      
      sector--;
    }
    cache_write(doubly_indirect_table->table[i], indirect_table, 512, 0);
    free(indirect_table);

  }
  cache_write(disk_inode->doubly_indirect, doubly_indirect_table, 512, 0);
  free(doubly_indirect_table);
  //printf("@@@@@@@@@@@@ allocate_sectors finish @@@@@@@@@@@\n");
  return true;
}

static void
deallocate_sectors(struct inode_disk *data)
{
  size_t i, j;
  size_t sectors = bytes_to_sectors(data->length);

  struct sector_table *doubly_indirect_table = calloc(1, sizeof(struct sector_table));
  cache_read(data->doubly_indirect, doubly_indirect_table, 512, 0);
  
  for(i = 0; i < 128; i++)
  {
    if(sectors == 0)
      break;
    struct sector_table *indirect_table = calloc(1, sizeof(struct sector_table));
    cache_read(data->indirect, indirect_table, 512, 0);
    
    for(j = 0; j < 128; j++)
    {
      if(sectors == 0)
        break;
      free_map_release(indirect_table->table[j], 1);
      sectors--;
    }
    free_map_release(data->indirect, 1);
    free(indirect_table);

  }
  free_map_release(data->doubly_indirect, 1);
  free(doubly_indirect_table);
}

/* Returns the disk sector that contains byte offset POS within
   INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
/*
byte_to_sector(inode, pos)
inode에 해당하는 파일에서 pos번째 값을 가지고 있는 정보가 실제 disk에 어느 sector에 들어있는지 확인한다
이미 pos번째에 해당하는 섹터가 할당되어있다면 테이블에서 그 섹터를 찾아서 return해주면 되고,
그렇지 않다면 allocate_sector을 호출해서 섹터를 할당해주고 다시 byte_to_sector를 실행해준다.
*/
static int
byte_to_sector(const struct inode *inode, off_t pos) 
{
  //printf("@@@@@@@@@@@@@@ byte_to_sector start pos : %d  @@@@@@@@@@@@@@@\n",pos);
  ASSERT (inode != NULL);
  struct inode_disk *inode_d = malloc(sizeof(struct inode_disk));
  int sector;
  //printf("aaaaaaaaaa\n");
  cache_read(inode->sector, inode_d, 512, 0);
  if(bytes_to_sectors(pos) > bytes_to_sectors(inode_d->length))
    sector = -1;
  else
  {
  struct sector_table *dit = malloc(sizeof(struct sector_table));
  cache_read(inode_d->doubly_indirect, dit, 512, 0);
  //printf("cccccccccccc\n");
  struct sector_table *it = malloc(sizeof(struct sector_table));
  int index = (pos / (128 * 512));
  cache_read(dit->table[index], it, 512, 0);
  //printf("dddddddddddd\n");
  sector = it->table[(pos - index * (128 * 512)) / 512];
  free(it);
  free(dit);
  }
  free(inode_d);
  return sector;
}


/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   disk.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length, bool isdir)
{
  //printf("@@@@@@@@@@@@@@@@@@ inode_create start sector : %d length : %d @@@@@@@@@@@@\n",sector,length);
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->isdir = isdir;
      disk_inode->magic = INODE_MAGIC;
      /*
      if (free_map_allocate (sectors, &disk_inode->start))
        {
          disk_write (filesys_disk, sector, disk_inode);
          if (sectors > 0) 
            {
              static char zeros[DISK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++) 
                disk_write (filesys_disk, disk_inode->start + i, zeros); 
            }
          success = true; 
        }*/
      if(allocate_sectors(disk_inode, sectors, 0))
      {
        //disk_write(filesys_disk, sector, disk_inode);
        cache_write(sector, disk_inode, 512, 0);
        //printf("@@@@@@@@ disk_inode -> disk @@@@@@@\n");
        success = true;
      }
      free (disk_inode);
    }
  //printf("@@@@@@@@@@ inode_create finish @@@@@@@@@\n");
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) 
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  //disk_read (filesys_disk, inode->sector, &inode->data);
  cache_read(inode->sector, &inode->data, 512, 0);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
          //free_map_release (inode->data.start,
          //                  bytes_to_sectors (inode->data.length)); 
          deallocate_sectors(&inode->data);
        }

      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  //uint8_t *bounce = NULL;
  //printf("@@@@@@@@@@@@ file_read start @@@@@@@@@@@\n");
  struct inode_disk *inode_d;
  inode_d = malloc(sizeof(struct inode_disk));
  while (size > 0) 
    {
      cache_read(inode->sector, inode_d, DISK_SECTOR_SIZE, 0);
      /* Disk sector to read, starting byte offset within sector. */
      int sector_idx = byte_to_sector (inode, offset);
      //printf("@@@@@@@@@ sector_idx : %d @@@@@@@@@@\n",sector_idx);
      if(sector_idx == -1) 
        break;
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      cache_read(sector_idx, buffer + bytes_read, chunk_size, sector_ofs);
      /*
      if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) 
        {
           Read full sector directly into caller's buffer. 
          disk_read (filesys_disk, sector_idx, buffer + bytes_read); 
        }
      else 
        {
           Read sector into bounce buffer, then partially copy
             into caller's buffer. 
          
          if (bounce == NULL) 
            {
              bounce = malloc (DISK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          disk_read (filesys_disk, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      */
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free(inode_d);
  //free (bounce);
  //printf("@@@@@@@@@@@@ file_read finish @@@@@@@@@@@\n");
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  //uint8_t *bounce = NULL;
  //struct inode_disk *temp = calloc(1, sizeof(struct inode_disk));

  if (inode->deny_write_cnt)
    return 0;
  struct inode_disk *inode_d;
  inode_d = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, inode_d, DISK_SECTOR_SIZE, 0);

  if(bytes_to_sectors(offset + size) > bytes_to_sectors(inode_d->length)) // 추가할당이 필요한 경우
  {
    if(!allocate_sectors(inode_d, bytes_to_sectors(offset+size), bytes_to_sectors(inode_d->length)))
      return 0;
    inode_d -> length = offset + size; //할당 완료시 length를 pos로 변경 
    cache_write (inode->sector, inode_d, 512, 0); //변경된 inode_d를 기록
  }
      
 // printf("@@@@@@@@@@@@ file_write start inode length : %d size : %d offset : %d  @@@@@@@@@@@@@@\n",inode_d->length,size,offset);
  while (size > 0) 
    {
      cache_read(inode->sector, inode_d, DISK_SECTOR_SIZE, 0);
      
      /* Sector to write, starting byte offset within sector. */
      int sector_idx = byte_to_sector (inode, offset);
      if(sector_idx == -1)
        break;
    //  printf("@@@@@@@@ find_sector : %d   @@@@@@@@@@@@@\n",sector_idx);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      //off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      //int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < sector_left ? size : sector_left;
      //printf("chunk_Size??????????????????:%d\n",chunk_size);
      if (chunk_size <= 0)
        break;
      //printf("@@@@@@@@@@@@@ write at : %d location : %x size :%d, offset : %d@@@@@@@@@@@@\n",sector_idx, buffer + bytes_written, chunk_size, sector_ofs);
      cache_write(sector_idx, buffer + bytes_written, chunk_size, sector_ofs);
      /*
      cache_read(inode->sector, temp, 512, 0);

      if(offset > temp->length)
      {
        temp->length = offset;
        cache_write(inode->sector, temp, 512, 0);
      }
      */
      /*
      if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) 
        {
          //Write full sector directly to disk. 
          //disk_write (filesys_disk, sector_idx, buffer + bytes_written); 
          cache_write(sector_idx, buffer + bytes_written, 512, 0);
        }
      else 
        {
          
           //We need a bounce buffer. 
          if (bounce == NULL) 
            {
              bounce = malloc (DISK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
           If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. 
          
          if (sector_ofs > 0 || chunk_size < sector_left) 
            disk_read (filesys_disk, sector_idx, bounce);
          else
            memset (bounce, 0, DISK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          disk_write (filesys_disk, sector_idx, bounce); 
          
          if(!(sector_ofs > 0 || chunk_size < sector_left))
            cache_write(sector_idx, 0, 512, 0);
          cache_write(sector_idx, buffer + bytes_written, chunk_size, sector_ofs);
        }*/
        
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
      //printf("%d %d\n",inode->sector,offset);
      if(offset > inode_d->length){
        inode_d->length = offset;
        //printf("update!!!!!");
        cache_write(inode->sector, inode_d, DISK_SECTOR_SIZE, 0);
      }
    }
  //free (bounce);
  free(inode_d);   
  //printf("@@@@@@@@@@@@ file_write finish @@@@@@@@@@@\n");
  //printf("%d\n",bytes_written);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk *inode_d;
  inode_d = malloc(sizeof(struct inode_disk));
  cache_read(inode->sector, inode_d, DISK_SECTOR_SIZE, 0); 
  int len = inode_d->length;
  free(inode_d);
  return len;
}
