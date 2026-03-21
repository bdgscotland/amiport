/*
 * amiport/unistd.h — POSIX unistd.h shim for AmigaOS
 *
 * Provides file I/O wrappers (open, close, read, write, lseek)
 * and miscellaneous POSIX functions mapped to AmigaOS equivalents.
 */

#ifndef AMIPORT_UNISTD_H
#define AMIPORT_UNISTD_H

#include <exec/types.h>

/* Standard file descriptor numbers */
#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

/* POSIX types not always available on AmigaOS.
 * Guard each with the same macros used by bebbo-gcc libnix sys/types.h
 * so there is no conflict when both headers are included. */
#ifndef _OFF_T_DECLARED
typedef long off_t;
#define _OFF_T_DECLARED
#endif

#ifndef _SSIZE_T_DECLARED
typedef long ssize_t;
#define _SSIZE_T_DECLARED
#endif

#ifndef __u_char_defined
typedef unsigned char u_char;
#define __u_char_defined
#endif

#ifndef __u_int_defined
typedef unsigned int u_int;
#define __u_int_defined
#endif

/* File access mode flags (matching POSIX values) */
#ifndef O_RDONLY
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#endif

/* Seek whence values */
#ifndef SEEK_SET
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif

/* File descriptor operations */
int  amiport_open(const char *path, int flags);
int  amiport_close(int fd);
LONG amiport_read(int fd, void *buf, LONG count);
LONG amiport_write(int fd, const void *buf, LONG count);
LONG amiport_lseek(int fd, LONG offset, int whence);

/* File descriptor duplication */
int  amiport_dup(int oldfd);
int  amiport_dup2(int oldfd, int newfd);

/* File operations */
int  amiport_unlink(const char *path);
int  amiport_rename(const char *oldpath, const char *newpath);
int  amiport_access(const char *path, int mode);

/* Process-like operations */
unsigned int amiport_sleep(unsigned int seconds);
char *amiport_getcwd(char *buf, int size);
int   amiport_chdir(const char *path);

/* Environment — returns malloc'd string, caller must free() */
char *amiport_getenv(const char *name);

/* Misc */
LONG  amiport_getpid(void);

/* access() mode flags */
#ifndef F_OK
#define F_OK    0   /* Test for existence */
#define R_OK    4   /* Test for read permission */
#define W_OK    2   /* Test for write permission */
#define X_OK    1   /* Test for execute permission */
#endif

/* isatty equivalent */
int amiport_isatty(int fd);

/* Thread-safe strtok (not provided by all Amiga C runtimes) */
char *amiport_strtok_r(char *str, const char *delim, char **saveptr);

/* Create a temporary file using T: assign.
 * Returns FILE* opened for read/write, or NULL on failure. */
#include <stdio.h>
FILE *amiport_tmpfile(void);

#endif /* AMIPORT_UNISTD_H */
