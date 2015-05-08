#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/init.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "devices/input.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

static uint32_t file_des = 3;

struct file_elem
{
    int fd;
    struct file *file;
    struct list_elem elem;
};

struct mmap_elem
{
	mapid_t map_des;
	struct file *file;
	void *start_addr;
	void *end_addr;
	struct list_elem elem;
};

static struct file *
fd_to_file(int fd)
{
	struct list_elem *e;
    struct file_elem *pfile_elem;
    struct thread *curr = thread_current();
    
    for(e = list_begin(&curr->file_list);
    	e != list_end(&curr->file_list); e = list_next(e))
    {
		pfile_elem = list_entry(e, struct file_elem, elem);

		if(pfile_elem->fd == fd)
	    	return pfile_elem->file;
	}
	return NULL;
}
    
void
syscall_init (void) 
{
  	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "sys call");
}
void
user_halt(void)
{
	power_off();
}
void
user_exit(int status)
{
	struct thread *curr = thread_current();
	struct list_elem *e;
	struct file_elem *pfile_elem;
	struct mmap_elem *m;
	
	for(e = list_begin(&curr->mmap);
    	e != list_end(&curr->mmap); e = list_begin(&curr->mmap))
	{
		m = list_entry(e, struct mmap_elem, elem);
	    user_munmap(m->map_des);
	}

	for(e = list_begin(&curr->file_list);
    	e != list_end(&curr->file_list); e = list_begin(&curr->file_list))
	{
		pfile_elem = list_entry(e, struct file_elem, elem);
	    user_close(pfile_elem->fd);
	}	

	printf("%s: exit(%d)\n", curr->name, status); 
	
 	curr->exit_status = status; //for waiting parent
	thread_exit();
}

pid_t
user_exec(const char *file)
{
	if(file == NULL)
	{
		return -1;
	}
	else
	{
		pid_t t = process_execute(file);
		
		struct thread *k = find_thread_by_tid(t);
		if(k == NULL) 
		{
			return -1;
		}
		while(!k->load_complete)
		{
			k->load_parent = thread_current();
			enum intr_level old_level = intr_disable();
			thread_block();
			intr_set_level(old_level);
			//k->load_parent = NULL;
		}
		if(k->load_success)
		{
			return t;
		}
		else
		{
			return -1;
		}
	}
}

int
user_wait(pid_t pid)
{
	return process_wait(pid);
}

bool
user_create(const char *file, unsigned size)
{
	
	if(file == NULL)
	{
		user_exit(-1);
	}
	else
	{
		bool temp = filesys_create(file, size);
		return temp;
	}
}
bool
user_remove(const char *file)
{
	
	if(file == NULL)
	{
		return false;
	}
	else
	{
		bool temp = filesys_remove(file);
		return temp;
	}
}
int
user_open(const char *file)
{
	
	if(file == NULL)
	{
		return -1;;
	}
	else
	{
		struct file *pfile;
		struct file_elem *pfile_elem = 
		(struct file_elem *) malloc(sizeof(struct file_elem));

		pfile = filesys_open(file);
		if(pfile == NULL)
		{
			return -1;
		}
		pfile_elem->file = pfile;
		pfile_elem->fd = file_des++;
		list_push_back(&thread_current()->file_list, &pfile_elem->elem);
		
		return pfile_elem->fd;
	}
}
int
user_filesize(int fd)
{
	
	struct file *f = fd_to_file(fd);
	if(f == NULL)
	{
		return -1;
	}
	int temp = file_length(f);
	return temp;
}

int
user_read(int fd, void *buffer, unsigned size)
{

	if (fd == 0)
	{
		unsigned i;
		for(i = 1; i < size; i++)
		{
			*(uint8_t *)(buffer + i) = input_getc();
		}
		return size;
	}
	else if(fd == 1 || fd < 0)
	{
		return -1;
	}
	else
	{
		struct file *k = fd_to_file(fd);
		if(k == NULL)
		{
			return -1;
		}
		int temp = file_read(k, buffer, size);
		return temp;
	}
}

int
user_write (int fd, const void *buffer, unsigned size)
{
	
	if(size<= 0)
	{
		return 0;
	}
	if(fd <= 0 || fd == 2)
	{
		return -1;
	}
	else if(fd == 1)
	{
		putbuf((const char *) buffer, size);
		return size;
	}
	else
	{
		struct file *file = fd_to_file(fd);
		if(file == NULL)
		{
			return -1;
		}
		int temp = file_write(file, buffer, size);
		return temp;
	}
}
void
user_seek(int fd, unsigned position)
{
	
	struct file *f = fd_to_file(fd);
	if(f != NULL)
	{
		file_seek(f, position);
	}
}

unsigned
user_tell(int fd)
{
	
	struct file *f = fd_to_file(fd);
	if(f != NULL)
	{
		unsigned temp = file_tell(f);
		return temp;
	}
	else
	{
		return -1;
	}
}

void
user_close(int fd)
{	
	struct list_elem *e;
    struct file_elem *pfile_elem;
    struct thread *curr = thread_current();
    
    for(e = list_begin(&curr->file_list);
    	e != list_end(&curr->file_list); e = list_next(e))
    {
		pfile_elem = list_entry(e, struct file_elem, elem);

		if(pfile_elem->fd == fd)
		{
			file_close(pfile_elem->file);
			list_remove(&pfile_elem->elem);
			free(pfile_elem);
			break;	    	
		}
	}
}

mapid_t
user_mmap (int fd, void *addr)
{
	struct thread *curr = thread_current();

	if(fd < 3 || (uintptr_t)addr & 0xfff ||
		(uintptr_t)addr == 0 || user_filesize(fd) == 0 ||
		pagedir_get_page(curr->pagedir, addr) != NULL ||
		spage_find(curr, addr) != NULL)
		return -1;
	else
	{
		mapid_t new_des;
		struct file *f = fd_to_file(fd);

		if(f == NULL)
			return -1;

		if(list_empty(&curr->mmap))
			new_des = 0;
		else
			new_des = list_entry(list_end(&curr->mmap), struct mmap_elem, elem)->map_des + 1;
		
		struct mmap_elem *m = (struct mmap_elem *) malloc(sizeof(struct mmap_elem));
		m->map_des = new_des;
		m->file = f;
		m->start_addr = addr;
		m->end_addr = addr + user_filesize(fd);
		list_push_back(&curr->mmap, &m->elem);
		return new_des;
	}

}

void
user_munmap (mapid_t mapping)
{
	struct list_elem *e;
    struct mmap_elem *m;
    struct thread *curr = thread_current();
    
    for(e = list_begin(&curr->mmap);
    	e != list_end(&curr->mmap); e = list_next(e))
    {
		m = list_entry(e, struct mmap_elem, elem);

		if(m->map_des == mapping)
		{
			//file_close(pfile_elem->file); // core part to implement.
			list_remove(&m->elem);
			free(m);
			break;	    	
		}
	}

}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  	uint32_t syscall_number = *(uintptr_t *)(f->esp);

  	if((uintptr_t)(f->esp + 4) >= 0xc0000000 || f->esp == NULL)
		user_exit(-1);
  	

  	switch (syscall_number)
  	{
  		case SYS_HALT : 
  			user_halt();
  		break;

  		case SYS_EXIT : 
  			user_exit(*(uintptr_t *)(f->esp + 4));
  		break;

 		case SYS_EXEC : 
  			f->eax = user_exec(*(const char **)(f->esp + 4));
  		break;

  		case SYS_WAIT : 
  			f->eax = user_wait(*(uintptr_t *)(f->esp + 4));
  		break;

		case SYS_CREATE : 
  			f->eax = user_create(*(const char **)(f->esp + 4),
  								*(uintptr_t *)(f->esp + 8));
  		break;

  		case SYS_REMOVE : 
  			f->eax = user_remove(*(const char **)(f->esp + 4));
  		break;

		case SYS_OPEN : 
  			f->eax = user_open(*(const char **)(f->esp + 4));
  		break;
	
		case SYS_FILESIZE : 
  			f->eax = user_filesize(*(uintptr_t *)(f->esp + 4));
  		break;  

  		case SYS_READ : 
  			f->eax = user_read(*(uintptr_t *)(f->esp + 4),
	    						*(char **)(f->esp + 8),
	    						*(uintptr_t *)(f->esp + 12));
  		break;

	    case SYS_WRITE :
	    	f->eax = user_write(*(uintptr_t *)(f->esp + 4),
	    						*(const char **)(f->esp + 8),
	    						*(uintptr_t *)(f->esp + 12));
	    break;

	    case SYS_SEEK : 
  			user_seek(*(uintptr_t *)(f->esp + 4),
  						*(unsigned *)(f->esp + 8)) ;
  		break;

		case SYS_TELL : 
  			f->eax = user_tell(*(uintptr_t *)(f->esp + 4));
  		break;

  		case SYS_CLOSE : 
  			user_close(*(uintptr_t *)(f->esp + 4));
  		break;

  		case SYS_MMAP :
  			f->eax = user_mmap(*(uintptr_t *)(f->esp + 4),
  							   *(char **)(f->esp + 8));
  		break;

  		case SYS_MUNMAP:
  			user_munmap(*(int *)(f->esp + 4));
  		break;

	    default : break;
  	}
}
