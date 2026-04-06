/*
 * json.h -- amiport manifest JSON parser
 *
 * Scoped to the amiport package manifest schema.
 * Static arrays, no malloc. Stream-prints rich fields for info command.
 *
 * amiport: original code for amiport
 */

#ifndef AMIPORT_JSON_H
#define AMIPORT_JSON_H

#include <stdio.h>

#define AMIPORT_MAX_PACKAGES 128

struct amiport_package {
    char name[32];
    char version[16];
    char description[80];
    char category[32];
    char download[128]; /* relative URL path, e.g. /packages/grep-1.68.lha */
    char sha256[65];
    char machine_sha256[65];
    long size;
    long stack;
    char requires[4][32];
    int  num_requires;
    /* Short fields for info display */
    char source[80];
    char license[48];
    /* Rich fields (porting_notes, known_limitations) are NOT stored --
     * they are stream-printed during info command JSON parsing */
};

/* Parse manifest JSON from file. Returns number of packages parsed, or -1.
 * Fills pkgs array up to max_pkgs. */
int amiport_parse_manifest(const char *path,
                          struct amiport_package *pkgs, int max_pkgs);

/* Parse single-package JSON from file and print info to stdout.
 * Short fields go into pkg struct. Rich fields (porting_notes,
 * known_limitations) are printed directly during parsing.
 * Returns 0 on success, -1 on error. */
int amiport_parse_package_info(const char *path,
                              struct amiport_package *pkg,
                              FILE *out);

#endif /* AMIPORT_JSON_H */
