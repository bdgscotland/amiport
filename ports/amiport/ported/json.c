/*
 * json.c -- amiport manifest JSON parser
 *
 * Hand-rolled parser scoped to amiport manifest schema.
 * Static arrays, no malloc. Handles pretty-printed JSON
 * (whitespace between keys and values).
 *
 * amiport: original code for amiport
 */

#include "json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* --- Internal helpers --- */

/* Skip whitespace */
static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

/* Extract a JSON string value. Advances *pp past the closing quote.
 * Copies at most maxlen-1 chars into out. Returns 0 on success. */
static int extract_string(const char **pp, char *out, int maxlen)
{
    const char *p = *pp;
    int i = 0;

    if (*p != '"') return -1;
    p++;

    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1)) {
            p++; /* skip escape */
            if (*p == 'n' && i < maxlen - 1) { out[i++] = '\n'; }
            else if (*p == 't' && i < maxlen - 1) { out[i++] = '\t'; }
            else if (*p == '"' && i < maxlen - 1) { out[i++] = '"'; }
            else if (*p == '\\' && i < maxlen - 1) { out[i++] = '\\'; }
            else if (*p == '/' && i < maxlen - 1) { out[i++] = '/'; }
            else if (i < maxlen - 1) { out[i++] = *p; }
            p++;
        } else {
            if (i < maxlen - 1) out[i++] = *p;
            p++;
        }
    }
    out[i] = '\0';

    if (*p == '"') p++;
    *pp = p;
    return 0;
}

/* Skip a JSON value (string, number, object, array, bool, null). */
static const char *skip_value(const char *p)
{
    int depth;

    p = skip_ws(p);
    if (*p == '"') {
        /* String: scan to unescaped quote */
        p++;
        while (*p && *p != '"') {
            if (*p == '\\' && *(p + 1)) p++;
            p++;
        }
        if (*p == '"') p++;
    } else if (*p == '{') {
        /* Object: count braces */
        depth = 1;
        p++;
        while (*p && depth > 0) {
            if (*p == '{') depth++;
            else if (*p == '}') depth--;
            else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\' && *(p + 1)) p++;
                    p++;
                }
            }
            if (*p) p++;
        }
    } else if (*p == '[') {
        /* Array: count brackets */
        depth = 1;
        p++;
        while (*p && depth > 0) {
            if (*p == '[') depth++;
            else if (*p == ']') depth--;
            else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\' && *(p + 1)) p++;
                    p++;
                }
            }
            if (*p) p++;
        }
    } else {
        /* Number, bool, null: scan to delimiter */
        while (*p && *p != ',' && *p != '}' && *p != ']' &&
               *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
            p++;
        }
    }
    return p;
}

/* Parse a long integer value */
static long parse_long(const char **pp)
{
    const char *p = *pp;
    long val = 0;

    p = skip_ws(p);
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    *pp = p;
    return val;
}

/* Parse a string array into requires[]. */
static int parse_requires(const char **pp, struct amiport_package *pkg)
{
    const char *p = *pp;

    p = skip_ws(p);
    if (*p != '[') return -1;
    p++;

    pkg->num_requires = 0;
    while (*p && *p != ']') {
        p = skip_ws(p);
        if (*p == '"' && pkg->num_requires < 4) {
            extract_string(&p, pkg->requires[pkg->num_requires], 32);
            pkg->num_requires++;
        } else {
            p = skip_value(p);
        }
        p = skip_ws(p);
        if (*p == ',') p++;
    }
    if (*p == ']') p++;
    *pp = p;
    return 0;
}

/* Parse one package object. Returns pointer past closing }. */
static const char *parse_package(const char *p, struct amiport_package *pkg)
{
    char key[64];

    memset(pkg, 0, sizeof(*pkg));

    p = skip_ws(p);
    if (*p != '{') return NULL;
    p++;

    while (*p && *p != '}') {
        p = skip_ws(p);
        if (*p != '"') { p = skip_value(p); continue; }

        /* Extract key */
        if (extract_string(&p, key, sizeof(key)) != 0) return NULL;

        p = skip_ws(p);
        if (*p == ':') p++;
        p = skip_ws(p);

        /* Match known keys */
        if (strcmp(key, "name") == 0) {
            extract_string(&p, pkg->name, sizeof(pkg->name));
        } else if (strcmp(key, "version") == 0) {
            extract_string(&p, pkg->version, sizeof(pkg->version));
        } else if (strcmp(key, "description") == 0) {
            extract_string(&p, pkg->description, sizeof(pkg->description));
        } else if (strcmp(key, "category") == 0) {
            extract_string(&p, pkg->category, sizeof(pkg->category));
        } else if (strcmp(key, "download") == 0) {
            extract_string(&p, pkg->download, sizeof(pkg->download));
        } else if (strcmp(key, "sha256") == 0) {
            extract_string(&p, pkg->sha256, sizeof(pkg->sha256));
        } else if (strcmp(key, "machine_sha256") == 0) {
            extract_string(&p, pkg->machine_sha256, sizeof(pkg->machine_sha256));
        } else if (strcmp(key, "source") == 0) {
            extract_string(&p, pkg->source, sizeof(pkg->source));
        } else if (strcmp(key, "license") == 0) {
            extract_string(&p, pkg->license, sizeof(pkg->license));
        } else if (strcmp(key, "size") == 0) {
            pkg->size = parse_long(&p);
        } else if (strcmp(key, "stack") == 0) {
            pkg->stack = parse_long(&p);
        } else if (strcmp(key, "requires") == 0) {
            parse_requires(&p, pkg);
        } else {
            /* Unknown key: skip value */
            p = skip_value(p);
        }

        p = skip_ws(p);
        if (*p == ',') p++;
    }

    if (*p == '}') p++;
    return p;
}

/* --- Public API --- */

int amiport_parse_manifest(const char *path,
                          struct amiport_package *pkgs, int max_pkgs)
{
    FILE *fp;
    static char buf[128 * 1024]; /* 128KB manifest buffer */
    size_t n;
    const char *p;
    int count = 0;

    fp = fopen(path, "r");
    if (!fp) return -1;

    n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    if (n == 0) return -1;
    buf[n] = '\0';

    /* Find "packages" array -- handle whitespace around : and [ */
    p = strstr(buf, "\"packages\"");
    if (!p) return -1;
    p += 10; /* skip "packages" */
    p = skip_ws(p);
    if (*p == ':') p++;
    p = skip_ws(p);
    if (*p != '[') return -1;
    p++;

    /* Parse each package object */
    while (*p && *p != ']' && count < max_pkgs) {
        p = skip_ws(p);
        if (*p == '{') {
            p = parse_package(p, &pkgs[count]);
            if (!p) return -1;
            if (pkgs[count].name[0]) count++;
        } else {
            break;
        }
        p = skip_ws(p);
        if (*p == ',') p++;
    }

    return count;
}

int amiport_parse_package_info(const char *path,
                              struct amiport_package *pkg,
                              FILE *out)
{
    FILE *fp;
    static char buf[16 * 1024]; /* 16KB for single package */
    size_t n;
    const char *p;
    char key[64];
    char rich_buf[512]; /* chunk buffer for stream printing */

    fp = fopen(path, "r");
    if (!fp) return -1;

    n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    if (n == 0) return -1;
    buf[n] = '\0';

    memset(pkg, 0, sizeof(*pkg));

    /* Find opening { */
    p = buf;
    p = skip_ws(p);

    /* Handle both {"name":...} and {"packages":[{"name":...}]} */
    if (*p == '{') {
        const char *outer = p; /* save opening { position */
        const char *test = p + 1;
        test = skip_ws(test);
        if (*test == '"') {
            char testkey[16];
            extract_string(&test, testkey, sizeof(testkey));
            if (strcmp(testkey, "packages") == 0) {
                /* Wrapped in packages array */
                test = skip_ws(test);
                if (*test == ':') test++;
                test = skip_ws(test);
                if (*test == '[') test++;
                test = skip_ws(test);
                p = test;
            } else {
                /* Flat object — reset to opening { */
                p = outer;
            }
        }
    }

    if (*p != '{') return -1;
    p++;

    while (*p && *p != '}') {
        p = skip_ws(p);
        if (*p != '"') { p = skip_value(p); p = skip_ws(p); if (*p == ',') p++; continue; }

        if (extract_string(&p, key, sizeof(key)) != 0) return -1;
        p = skip_ws(p);
        if (*p == ':') p++;
        p = skip_ws(p);

        /* Stream-print rich fields directly */
        if (strcmp(key, "porting_notes") == 0 ||
            strcmp(key, "known_limitations") == 0) {
            if (*p == '"') {
                const char *label;
                if (strcmp(key, "porting_notes") == 0)
                    label = "Porting Notes";
                else
                    label = "Limitations";
                fprintf(out, "  %-16s ", label);
                p++; /* skip opening quote */
                while (*p && *p != '"') {
                    if (*p == '\\' && *(p + 1)) {
                        p++;
                        if (*p == 'n') fputc('\n', out);
                        else if (*p == 't') fputc('\t', out);
                        else fputc(*p, out);
                    } else {
                        fputc(*p, out);
                    }
                    p++;
                }
                if (*p == '"') p++;
                fputc('\n', out);
            } else {
                p = skip_value(p);
            }
        } else if (strcmp(key, "name") == 0) {
            extract_string(&p, pkg->name, sizeof(pkg->name));
        } else if (strcmp(key, "version") == 0) {
            extract_string(&p, pkg->version, sizeof(pkg->version));
        } else if (strcmp(key, "description") == 0) {
            extract_string(&p, pkg->description, sizeof(pkg->description));
        } else if (strcmp(key, "category") == 0) {
            extract_string(&p, pkg->category, sizeof(pkg->category));
        } else if (strcmp(key, "download") == 0) {
            extract_string(&p, pkg->download, sizeof(pkg->download));
        } else if (strcmp(key, "sha256") == 0) {
            extract_string(&p, pkg->sha256, sizeof(pkg->sha256));
        } else if (strcmp(key, "machine_sha256") == 0) {
            extract_string(&p, pkg->machine_sha256, sizeof(pkg->machine_sha256));
        } else if (strcmp(key, "source") == 0) {
            extract_string(&p, pkg->source, sizeof(pkg->source));
        } else if (strcmp(key, "license") == 0) {
            extract_string(&p, pkg->license, sizeof(pkg->license));
        } else if (strcmp(key, "size") == 0) {
            pkg->size = parse_long(&p);
        } else if (strcmp(key, "stack") == 0) {
            pkg->stack = parse_long(&p);
        } else if (strcmp(key, "requires") == 0) {
            parse_requires(&p, pkg);
        } else {
            p = skip_value(p);
        }

        p = skip_ws(p);
        if (*p == ',') p++;
    }

    (void)rich_buf; /* reserved for future line-wrapping */
    return (pkg->name[0]) ? 0 : -1;
}
