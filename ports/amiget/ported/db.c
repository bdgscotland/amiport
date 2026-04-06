/*
 * db.c -- Installed package tracking database
 *
 * Read-modify-write S:amiget.db with temp file + AmigaDOS Rename.
 *
 * amiport: original code for amiget
 */

#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __AMIGA__
#include <proto/dos.h>
#endif

#define DB_PATH "S:amiget.db"
#define DB_TMP  "S:amiget.db.tmp"

/* Static storage for find results */
static struct amiget_installed db_find_result;

/* Static storage for full DB */
static struct amiget_installed db_entries[AMIGET_MAX_INSTALLED];
static int db_count = 0;
static int db_loaded = 0; /* perf: cache DB reads across find() calls */

/* Load DB from file into static array. */
static int db_reload(void)
{
    FILE *fp;
    char line[128];

    if (db_loaded) return db_count; /* already cached this invocation */

    db_count = 0;
    fp = fopen(DB_PATH, "r");
    if (!fp) { db_loaded = 1; return 0; } /* no DB file = 0 entries */

    while (fgets(line, sizeof(line), fp) && db_count < AMIGET_MAX_INSTALLED) {
        char *name_end;
        char *ver_end;
        char *path_start;
        int len;

        /* Strip trailing newline */
        len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;

        /* Parse: name version path */
        name_end = strchr(line, ' ');
        if (!name_end) continue;
        *name_end = '\0';

        ver_end = strchr(name_end + 1, ' ');
        if (!ver_end) continue;
        *ver_end = '\0';

        path_start = ver_end + 1;

        /* Copy to entry */
        strncpy(db_entries[db_count].name, line,
                sizeof(db_entries[db_count].name) - 1);
        db_entries[db_count].name[sizeof(db_entries[db_count].name) - 1] = '\0';

        strncpy(db_entries[db_count].version, name_end + 1,
                sizeof(db_entries[db_count].version) - 1);
        db_entries[db_count].version[sizeof(db_entries[db_count].version) - 1] = '\0';

        strncpy(db_entries[db_count].path, path_start,
                sizeof(db_entries[db_count].path) - 1);
        db_entries[db_count].path[sizeof(db_entries[db_count].path) - 1] = '\0';

        db_count++;
    }

    fclose(fp);
    db_loaded = 1;
    return db_count;
}

/* Write static array back to DB file via temp+rename. */
static int db_flush(void)
{
    FILE *fp;
    int i;

    fp = fopen(DB_TMP, "w");
    if (!fp) return -1;

    for (i = 0; i < db_count; i++) {
        fprintf(fp, "%s %s %s\n",
                db_entries[i].name,
                db_entries[i].version,
                db_entries[i].path);
    }

    /* Check fclose return for disk-full detection */
    if (fclose(fp) != 0) {
        remove(DB_TMP);
        return -1;
    }

    /* Atomic rename */
#ifdef __AMIGA__
    /* AmigaDOS Rename overwrites the destination */
    remove(DB_PATH); /* ignore error -- may not exist */
    if (!Rename((CONST_STRPTR)DB_TMP, (CONST_STRPTR)DB_PATH)) {
        remove(DB_TMP);
        return -1;
    }
#else
    remove(DB_PATH);
    if (rename(DB_TMP, DB_PATH) != 0) {
        remove(DB_TMP);
        return -1;
    }
#endif

    db_loaded = 0; /* perf: invalidate cache after write */
    return 0;
}

/* --- Public API --- */

int amiget_db_load(struct amiget_installed *entries, int max_entries)
{
    int i;
    int rc;

    rc = db_reload();
    if (rc < 0) return -1;

    for (i = 0; i < db_count && i < max_entries; i++) {
        entries[i] = db_entries[i];
    }
    return (db_count < max_entries) ? db_count : max_entries;
}

int amiget_db_save(const char *name, const char *version, const char *path)
{
    int i;

    db_reload();

    /* Update existing entry or add new */
    for (i = 0; i < db_count; i++) {
        if (strcmp(db_entries[i].name, name) == 0) {
            strncpy(db_entries[i].version, version,
                    sizeof(db_entries[i].version) - 1);
            db_entries[i].version[sizeof(db_entries[i].version) - 1] = '\0';
            strncpy(db_entries[i].path, path,
                    sizeof(db_entries[i].path) - 1);
            db_entries[i].path[sizeof(db_entries[i].path) - 1] = '\0';
            return db_flush();
        }
    }

    /* New entry */
    if (db_count >= AMIGET_MAX_INSTALLED) return -1;

    strncpy(db_entries[db_count].name, name,
            sizeof(db_entries[db_count].name) - 1);
    db_entries[db_count].name[sizeof(db_entries[db_count].name) - 1] = '\0';
    strncpy(db_entries[db_count].version, version,
            sizeof(db_entries[db_count].version) - 1);
    db_entries[db_count].version[sizeof(db_entries[db_count].version) - 1] = '\0';
    strncpy(db_entries[db_count].path, path,
            sizeof(db_entries[db_count].path) - 1);
    db_entries[db_count].path[sizeof(db_entries[db_count].path) - 1] = '\0';
    db_count++;

    return db_flush();
}

int amiget_db_remove(const char *name)
{
    int i;
    int found = 0;

    db_reload();

    for (i = 0; i < db_count; i++) {
        if (strcmp(db_entries[i].name, name) == 0) {
            /* Shift remaining entries down */
            if (i < db_count - 1) {
                memmove(&db_entries[i], &db_entries[i + 1],
                        (db_count - i - 1) * sizeof(struct amiget_installed));
            }
            db_count--;
            found = 1;
            break;
        }
    }

    if (!found) return -1;
    return db_flush();
}

const struct amiget_installed *amiget_db_find(const char *name)
{
    int i;

    db_reload();

    for (i = 0; i < db_count; i++) {
        if (strcmp(db_entries[i].name, name) == 0) {
            db_find_result = db_entries[i];
            return &db_find_result;
        }
    }
    return NULL;
}
