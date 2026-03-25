/*
 * popen.c -- popen/pclose/system emulation for AmigaOS
 *
 * Uses temp files + dos.library/SystemTagList() to emulate POSIX
 * popen/pclose. NOT concurrent -- commands run synchronously.
 *
 * Strategy:
 *   popen(cmd, "r"): Run cmd with stdout -> T:tmpfile, return fopen(T:tmpfile, "r")
 *   popen(cmd, "w"): Return fopen(T:tmpfile, "w"), run cmd on pclose with stdin <- T:tmpfile
 *   system(cmd):     Direct SystemTagList(cmd, NULL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dostags.h>

#define MAX_POPEN_ENTRIES 16
#define TMPFILE_PREFIX "T:amiport_popen_"

/* amiport: tracking table for open popen streams */
struct popen_entry {
    FILE *fp;
    char *cmd;          /* stored command (for "w" mode deferred execution) */
    char tmpfile[48];   /* T:amiport_popen_NNNN */
    char mode;          /* 'r' or 'w' */
    int in_use;
};

static struct popen_entry popen_table[MAX_POPEN_ENTRIES];
static int popen_counter = 0;

static struct popen_entry *find_entry(FILE *fp)
{
    int i;
    for (i = 0; i < MAX_POPEN_ENTRIES; i++) {
        if (popen_table[i].in_use && popen_table[i].fp == fp)
            return &popen_table[i];
    }
    return NULL;
}

static struct popen_entry *alloc_entry(void)
{
    int i;
    for (i = 0; i < MAX_POPEN_ENTRIES; i++) {
        if (!popen_table[i].in_use) {
            memset(&popen_table[i], 0, sizeof(struct popen_entry));
            popen_table[i].in_use = 1;
            return &popen_table[i];
        }
    }
    return NULL;
}

static void free_entry(struct popen_entry *e)
{
    if (e->cmd) {
        free(e->cmd);
        e->cmd = NULL;
    }
    e->fp = NULL;
    e->in_use = 0;
}

/*
 * Run a command with optional input/output redirection via temp files.
 * Returns the command's exit status, or -1 on failure.
 */
static int run_command(const char *cmd, const char *input_file, const char *output_file)
{
    BPTR fh_in = 0;
    BPTR fh_out = 0;
    LONG result;
    struct TagItem tags[4];
    int tag_idx = 0;

    if (input_file) {
        fh_in = Open((STRPTR)input_file, MODE_OLDFILE);
        if (!fh_in)
            return -1;
        tags[tag_idx].ti_Tag = SYS_Input;
        tags[tag_idx].ti_Data = (ULONG)fh_in;
        tag_idx++;
    }

    if (output_file) {
        fh_out = Open((STRPTR)output_file, MODE_NEWFILE);
        if (!fh_out) {
            if (fh_in) Close(fh_in);
            return -1;
        }
        tags[tag_idx].ti_Tag = SYS_Output;
        tags[tag_idx].ti_Data = (ULONG)fh_out;
        tag_idx++;
    }

    tags[tag_idx].ti_Tag = TAG_DONE;
    tags[tag_idx].ti_Data = 0;

    result = SystemTagList((STRPTR)cmd, tags);

    /* SystemTagList does NOT close the filehandles -- we must do it */
    if (fh_in) Close(fh_in);
    if (fh_out) Close(fh_out);

    return (int)result;
}

FILE *amiport_emu_popen(const char *command, const char *mode)
{
    struct popen_entry *e;
    int status;

    if (!command || !mode) {
        errno = EINVAL;
        return NULL;
    }
    if (mode[0] != 'r' && mode[0] != 'w') {
        errno = EINVAL;
        return NULL;
    }

    e = alloc_entry();
    if (!e) {
        errno = EMFILE;
        return NULL;
    }

    /* Generate unique temp filename */
    popen_counter++;
    sprintf(e->tmpfile, "%s%04d", TMPFILE_PREFIX, popen_counter);
    e->mode = mode[0];

    if (mode[0] == 'r') {
        /* "r" mode: run command NOW, capture output to temp file */
        status = run_command(command, NULL, e->tmpfile);
        if (status == -1) {
            /* Command failed to execute -- but temp file may exist empty */
        }
        /* Open the temp file for reading */
        e->fp = fopen(e->tmpfile, "r");
        if (!e->fp) {
            DeleteFile((STRPTR)e->tmpfile);
            free_entry(e);
            errno = EIO;
            return NULL;
        }
        /* Store status for pclose to return */
        e->cmd = NULL;
        /* Store the exit status in a hacky way -- reuse cmd pointer space */
        /* Actually, just use a union or store directly. Let's use cmd as int storage. */
        e->cmd = malloc(sizeof(int));
        if (e->cmd)
            *(int *)e->cmd = status;
    } else {
        /* "w" mode: open temp file for writing, defer command to pclose */
        e->fp = fopen(e->tmpfile, "w");
        if (!e->fp) {
            free_entry(e);
            errno = EIO;
            return NULL;
        }
        e->cmd = strdup(command);
        if (!e->cmd) {
            fclose(e->fp);
            DeleteFile((STRPTR)e->tmpfile);
            free_entry(e);
            errno = ENOMEM;
            return NULL;
        }
    }

    return e->fp;
}

int amiport_emu_pclose(FILE *stream)
{
    struct popen_entry *e;
    int status = -1;

    if (!stream) {
        errno = EINVAL;
        return -1;
    }

    e = find_entry(stream);
    if (!e) {
        /* Not a popen'd stream -- just fclose it */
        errno = EINVAL;
        return -1;
    }

    if (e->mode == 'r') {
        /* "r" mode: command already ran. Retrieve stored status. */
        if (e->cmd)
            status = *(int *)e->cmd;
        fclose(e->fp);
    } else {
        /* "w" mode: close the temp file, then run the command with it as input */
        fclose(e->fp);
        if (e->cmd)
            status = run_command(e->cmd, e->tmpfile, NULL);
    }

    /* Clean up temp file */
    DeleteFile((STRPTR)e->tmpfile);
    free_entry(e);

    return status;
}

int amiport_emu_system(const char *command)
{
    LONG result;

    if (!command) {
        /* POSIX: system(NULL) returns non-zero if a command processor is available */
        return 1;
    }

    result = SystemTagList((STRPTR)command, NULL);
    return (int)result;
}
