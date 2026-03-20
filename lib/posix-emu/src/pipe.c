/*
 * pipe.c — pipe() emulation via PIPE: device
 *
 * Tier 2 emulation: approximate POSIX semantics with documented differences.
 * See amiport-emu/pipe.h for the emulation notice.
 */

#include <amiport-emu/pipe.h>

#if AMIPORT_EMU_PIPE_ENABLED

#include <amiport/unistd.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <errno.h>
#include <stdio.h>

/* Counter for unique pipe names */
static unsigned long pipe_counter = 0;

int amiport_emu_pipe(int pipefd[2])
{
    char pipe_name[48];
    int read_fd, write_fd;

    /* Generate a unique PIPE: name */
    sprintf(pipe_name, "PIPE:amiport_%08lx_%04lx",
            (unsigned long)FindTask(NULL), pipe_counter++);

    /* Open write end first (creates the pipe) */
    write_fd = amiport_open(pipe_name, 1 /* O_WRONLY */);
    if (write_fd < 0) {
        errno = EMFILE;
        return -1;
    }

    /* Open read end */
    read_fd = amiport_open(pipe_name, 0 /* O_RDONLY */);
    if (read_fd < 0) {
        amiport_close(write_fd);
        errno = EMFILE;
        return -1;
    }

    pipefd[0] = read_fd;
    pipefd[1] = write_fd;
    return 0;
}

#endif /* AMIPORT_EMU_PIPE_ENABLED */
