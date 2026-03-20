/*
 * amiport-emu/mmap.h — Read-only mmap() emulation for AmigaOS
 *
 * EMULATION NOTICE:
 * - Entire file is read into memory upfront (no lazy page faulting)
 * - Memory usage equals file size (no page sharing, no copy-on-write)
 * - File changes after mmap are not visible (snapshot semantics only)
 * - Only MAP_PRIVATE with PROT_READ is supported
 * - MAP_SHARED with write-back is NOT supported — needs Tier 3 redesign
 *
 * See docs/posix-tiers.md "Tier 2 — mmap()" for full details.
 */

#ifndef AMIPORT_EMU_MMAP_H
#define AMIPORT_EMU_MMAP_H

#ifndef AMIPORT_EMU_MMAP_ENABLED
#define AMIPORT_EMU_MMAP_ENABLED 1
#endif

#if AMIPORT_EMU_MMAP_ENABLED

/* mmap protection flags */
#define AMIPORT_EMU_PROT_NONE   0x0
#define AMIPORT_EMU_PROT_READ   0x1
#define AMIPORT_EMU_PROT_WRITE  0x2  /* supported only for MAP_PRIVATE */
#define AMIPORT_EMU_PROT_EXEC   0x4  /* ignored on Amiga */

/* mmap flags */
#define AMIPORT_EMU_MAP_SHARED  0x1  /* NOT SUPPORTED — returns MAP_FAILED */
#define AMIPORT_EMU_MAP_PRIVATE 0x2
#define AMIPORT_EMU_MAP_ANON    0x4  /* anonymous mapping (no file) */

#define AMIPORT_EMU_MAP_FAILED  ((void *)-1)

/*
 * amiport_emu_mmap() — Emulated mmap() via AllocMem()+Read().
 *
 * addr:   Ignored (always NULL on Amiga)
 * length: Number of bytes to map
 * prot:   Protection flags (only PROT_READ reliably supported)
 * flags:  MAP_PRIVATE only; MAP_SHARED returns MAP_FAILED
 * fd:     amiport file descriptor to map
 * offset: Offset into the file
 *
 * Returns: Pointer to mapped memory, or MAP_FAILED on error
 */
void *amiport_emu_mmap(void *addr, unsigned long length, int prot,
                       int flags, int fd, long offset);

/*
 * amiport_emu_munmap() — Free a mapping created by amiport_emu_mmap().
 *
 * Returns: 0 on success, -1 on error
 */
int amiport_emu_munmap(void *addr, unsigned long length);

#endif /* AMIPORT_EMU_MMAP_ENABLED */

#endif /* AMIPORT_EMU_MMAP_H */
