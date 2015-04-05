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

static void syscall_handler (struct intr_frame *);

/* structure for open() */
static uint32_t file_des = 2;
struct file_elem{
    int fd;
    struct file *file;
    struct list_elem elem;
};

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
	printf("%s: exit(%d)\n", curr->name, status);
	
 	curr->exit_status = status;
	thread_exit();
}

pid_t
user_exec(const char *file)
{
	return process_execute(file);
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
		user_exit(-1);
	else
		return filesys_create(file, size);
}
bool
user_remove(const char *file)
{
	if(file == NULL)
		user_exit(-1);
	else
		return filesys_remove(file);
}
int
user_open(const char *file)
{
	if(file == NULL)
		user_exit(-1);
	else
	{
		struct file *pfile;
		//struct file_elem *pfile_elem;

		pfile = filesys_open(file);
		if(pfile == NULL)
			return -1;
		//pfile_elem->file = pfile;
		//pfile_elem->fd = file_des++;
		return 2;
	}
}
int
user_filesize(int fd)
{
	return -1;
}
int
user_read(int fd, void *buffer, unsigned size)
{
	return -1;
}

int
user_write (int fd, const void *buffer, unsigned size)
{
	int written;

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
		written = file_write(fd, buffer, size);
		return written;
	}
}
void
user_seek(int fd, unsigned position)
{
}

unsigned
user_tell(int fd)
{
	return 0;
}

void
user_close(int fd)
{

}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	//printf ("system call!\n");
  	uint32_t syscall_number = *(uintptr_t *)(f->esp);
  	//printf("syscall_number:%d\n", syscall_number);
  	
  	//hex_dump(f->esp, f->esp, 256, true);
  	
  	if((f->esp + 4) >= 0xc0000000 || f->esp == NULL)
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
  			f->eax = user_exec(*(const char **)(f->esp + 4));
  		break;

		case SYS_OPEN : 
  			f->eax = user_open(*(const char **)(f->esp + 4));
  		break;
	
		case SYS_FILESIZE : 
  			f->eax = user_filesize(user_open(*(uintptr_t *)(f->esp + 4)));
  		break;  

  		case SYS_READ : 
  			f->eax = user_read(*(uintptr_t *)(f->esp + 4),
	    						*(const char **)(f->esp + 8),
	    						*(uintptr_t *)(f->esp + 12));
  		break;

	    case SYS_WRITE :
	    	f->eax = user_write(*(uintptr_t *)(f->esp + 4),
	    						*(const char **)(f->esp + 8),
	    						*(uintptr_t *)(f->esp + 12));
	    break;

	    case SYS_SEEK : 
  			user_seek(*(uintptr_t *)(f->esp + 4),
  						*(unsigned *)(f->esp + 4)) ;
  		break;

		case SYS_TELL : 
  			f->eax = user_tell(*(uintptr_t *)(f->esp + 4));
  		break;

  		case SYS_CLOSE : 
  			user_close(*(uintptr_t *)(f->esp + 4));
  		break;

	    default : break;
  	}
  	//thread_exit ();
}