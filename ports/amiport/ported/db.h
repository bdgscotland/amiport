/*
 * db.h -- Installed package tracking database
 *
 * Flat text file at S:amiport.db. One line per package.
 * Format: name version path (space-delimited, LF terminated).
 * Read-modify-write with temp+rename for crash safety.
 *
 * amiport: original code for amiport
 */

#ifndef AMIPORT_DB_H
#define AMIPORT_DB_H

#define AMIPORT_MAX_INSTALLED 128

struct amiport_installed {
    char name[32];
    char version[16];
    char path[64];
};

/* Load installed packages from S:amiport.db.
 * Returns count (0 if no DB file), or -1 on read error. */
int amiport_db_load(struct amiport_installed *entries, int max_entries);

/* Add or update an installed package entry.
 * Uses temp+rename for crash safety. Returns 0 on success, -1 on error. */
int amiport_db_save(const char *name, const char *version, const char *path);

/* Remove an installed package entry by name.
 * Returns 0 on success, -1 if not found or write error. */
int amiport_db_remove(const char *name);

/* Find an installed package by name.
 * Returns pointer to static entry, or NULL if not found. */
const struct amiport_installed *amiport_db_find(const char *name);

#endif /* AMIPORT_DB_H */
