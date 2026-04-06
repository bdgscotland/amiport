/*
 * config.h -- S:amiport.conf configuration file parser
 *
 * KEY=VALUE format, # comments, unknown keys ignored.
 * Missing file = use defaults. Malformed lines skipped.
 *
 * amiport: original code for amiport
 */

#ifndef AMIPORT_CONFIG_H
#define AMIPORT_CONFIG_H

struct amiport_config {
    char server[128];      /* Base URL, no trailing slash */
    char installpath[64];  /* Default: C: */
    int  color;            /* 1 = on (default), 0 = off */
};

/* Load config from S:amiport.conf. Missing file = defaults.
 * Always succeeds (fills defaults on any error). */
void amiport_config_load(struct amiport_config *cfg);

#endif /* AMIPORT_CONFIG_H */
