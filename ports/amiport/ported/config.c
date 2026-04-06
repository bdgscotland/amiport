/*
 * config.c -- S:amiport.conf configuration file parser
 *
 * Simple KEY=VALUE per line. # comments. Unknown keys ignored.
 * Splits on first = only (values may contain =).
 * Strips trailing slash from server URL.
 * Rejects https:// server URLs with a warning.
 *
 * amiport: original code for amiport
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define CONFIG_PATH "S:amiport.conf"
#define DEFAULT_SERVER "http://amiport.platesteel.net"
#define DEFAULT_INSTALLPATH "C:"

static void set_defaults(struct amiport_config *cfg)
{
    strcpy(cfg->server, DEFAULT_SERVER);
    strcpy(cfg->installpath, DEFAULT_INSTALLPATH);
    cfg->color = 1;
}

/* Strip trailing slashes from a string */
static void strip_trailing_slash(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && s[len - 1] == '/') {
        s[--len] = '\0';
    }
}

void amiport_config_load(struct amiport_config *cfg)
{
    FILE *fp;
    char line[256];
    char *eq;
    char *key;
    char *value;
    int len;

    set_defaults(cfg);

    fp = fopen(CONFIG_PATH, "r");
    if (!fp) return; /* missing file = use defaults */

    while (fgets(line, sizeof(line), fp)) {
        /* Strip newline */
        len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        /* Skip empty lines and comments */
        if (len == 0 || line[0] == '#') continue;

        /* Split on first = */
        eq = strchr(line, '=');
        if (!eq) continue; /* malformed, skip */

        *eq = '\0';
        key = line;
        value = eq + 1;

        if (strcmp(key, "server") == 0) {
            /* Reject https:// */
            if (strncmp(value, "https://", 8) == 0) {
                fprintf(stderr, "amiport: warning: HTTPS not supported "
                        "in server URL, using default\n");
                continue;
            }
            strncpy(cfg->server, value, sizeof(cfg->server) - 1);
            cfg->server[sizeof(cfg->server) - 1] = '\0';
            strip_trailing_slash(cfg->server);
        } else if (strcmp(key, "installpath") == 0) {
            strncpy(cfg->installpath, value, sizeof(cfg->installpath) - 1);
            cfg->installpath[sizeof(cfg->installpath) - 1] = '\0';
        } else if (strcmp(key, "color") == 0) {
            if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0) {
                cfg->color = 0;
            } else {
                cfg->color = 1;
            }
        }
        /* Unknown keys: silently ignored */
    }

    fclose(fp);
}
