#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/user/syscall.h"

void syscall_init (void);


void user_halt (void) NO_RETURN;
void user_exit (int status) NO_RETURN;
pid_t user_exec (const char *file);
int user_wait (pid_t);
bool user_create (const char *file, unsigned initial_size);
bool user_remove (const char *file);
int user_open (const char *file);
int user_filesize (int fd);
int user_read (int fd, void *buffer, unsigned length);
int user_write (int fd, const void *buffer, unsigned length);
void user_seek (int fd, unsigned position);
unsigned user_tell (int fd);
void user_close (int fd);

/* Project 3 and optionally project 4. */
mapid_t user_mmap (int fd, void *addr);
void user_munmap (mapid_t);

/* Project 4 only. */
bool user_chdir (const char *dir);
bool user_mkdir (const char *dir);
bool user_readdir (int fd, char name[READDIR_MAX_LEN + 1]);
bool user_isdir (int fd);
int user_inumber (int fd);

#endif /* userprog/syscall.h */
