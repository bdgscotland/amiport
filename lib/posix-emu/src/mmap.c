/*
 * mmap.c — Read-only mmap() emulation via AllocMem()+Read()
 *
 * Tier 2 emulation: approximate POSIX semantics with documented differences.
 * See amiport-emu/mmap.h for the emulation notice.
 */

#include <amiport-emu/mmap.h>

#if AMIPORT_EMU_MMAP_ENABLED

#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>

/* Access the Tier 1 shim fd table to get BPTR file handles */
extern BPTR amiport_fd_to_fh(int fd);

/*
 * Simple tracking of active mappings so munmap() can free them.
 * Limited to 16 concurrent mappings — sufficient for most ported code.
 */
#define MAX_MAPPINGS 16

struct mmap_entry {
    void *addr;
    unsigned long length;
};

static struct mmap_entry mappings[MAX_MAPPINGS] = { {0} };

void *amiport_emu_mmap(void *addr, unsigned long length, int prot,
                       int flags, int fd, long offset)
{
    BPTR fh;
    void *buf;
    LONG bytes_read;
    int i;

    (void)addr; /* ignored — we always choose the address */
    (void)prot; /* read-only enforced regardless */

    /* MAP_SHARED is not supported */
    if (flags & AMIPORT_EMU_MAP_SHARED) {
        errno = ENOTSUP;
        return AMIPORT_EMU_MAP_FAILED;
    }

    /* Anonymous mapping: just allocate zeroed memory */
    if (flags & AMIPORT_EMU_MAP_ANON) {
        buf = AllocVec(length, MEMF_CLEAR | MEMF_PUBLIC);
        if (!buf) {
            errno = ENOMEM;
            return AMIPORT_EMU_MAP_FAILED;
        }
    } else {
        /* File-backed mapping */
        fh = amiport_fd_to_fh(fd);
        if (fh == 0) {
            errno = EBADF;
            return AMIPORT_EMU_MAP_FAILED;
        }

        buf = AllocVec(length, MEMF_PUBLIC);
        if (!buf) {
            errno = ENOMEM;
            return AMIPORT_EMU_MAP_FAILED;
        }

        /* Seek to offset */
        if (offset != 0) {
            Seek(fh, offset, OFFSET_BEGINNING);
        }

        /* Read entire region into buffer */
        bytes_read = Read(fh, buf, length);
        if (bytes_read < 0) {
            FreeVec(buf);
            errno = EIO;
            return AMIPORT_EMU_MAP_FAILED;
        }

        /* Zero-fill any remainder if file was shorter than requested */
        if ((unsigned long)bytes_read < length) {
            char *p = (char *)buf;
            unsigned long j;
            for (j = (unsigned long)bytes_read; j < length; j++) {
                p[j] = 0;
            }
        }
    }

    /* Track the mapping */
    for (i = 0; i < MAX_MAPPINGS; i++) {
        if (mappings[i].addr == NULL) {
            mappings[i].addr = buf;
            mappings[i].length = length;
            break;
        }
    }

    return buf;
}

int amiport_emu_munmap(void *addr, unsigned long length)
{
    int i;

    (void)length;

    for (i = 0; i < MAX_MAPPINGS; i++) {
        if (mappings[i].addr == addr) {
            FreeVec(addr);
            mappings[i].addr = NULL;
            mappings[i].length = 0;
            return 0;
        }
    }

    errno = EINVAL;
    return -1;
}

#endif /* AMIPORT_EMU_MMAP_ENABLED */
