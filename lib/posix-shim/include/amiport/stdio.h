/*
 * amiport/stdio.h — Minimal stdio wrapper for AmigaOS
 *
 * Provides fileno() macro and fopen/fclose wrappers for amiport
 */

#ifndef AMIPORT_STDIO_H
#define AMIPORT_STDIO_H

#include <stdio.h>

/*
 * FILE* to fd mapping (for fileno() support)
 */

typedef struct {
    FILE *fp;
    int   fd;
} amiport_file_entry;

#define AMIPORT_MAX_FILE_ENTRIES 64

extern amiport_file_entry amiport_files[AMIPORT_MAX_FILE_ENTRIES];
extern int amiport_file_count;

/* Exported functions */
FILE *amiport_fopen(const char *path, const char *mode);
int amiport_fclose(FILE *fp);

static inline int
amiport_fileno(FILE *fp)
{
    int i;
    if (!fp) return -1;
    for (i = 0; i < amiport_file_count; i++) {
        if (amiport_files[i].fp == fp)
            return amiport_files[i].fd;
    }
    /* If not found, try to get from system fileno if it exists */
    return -1;
}

#ifndef AMIPORT_NO_STDIO_MACROS
#define fileno(fp)  amiport_fileno(fp)
#define fopen(p, m) amiport_fopen(p, m)
#define fclose(fp)  amiport_fclose(fp)
#endif

#endif /* AMIPORT_STDIO_H */
