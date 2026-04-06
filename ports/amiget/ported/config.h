/*
 * config.h -- S:amiget.conf configuration file parser
 *
 * KEY=VALUE format, # comments, unknown keys ignored.
 * Missing file = use defaults. Malformed lines skipped.
 *
 * amiport: original code for amiget
 */

#ifndef AMIGET_CONFIG_H
#define AMIGET_CONFIG_H

struct amiget_config {
    char server[128];      /* Base URL, no trailing slash */
    char installpath[64];  /* Default: C: */
    int  color;            /* 1 = on (default), 0 = off */
};

/* Load config from S:amiget.conf. Missing file = defaults.
 * Always succeeds (fills defaults on any error). */
void amiget_config_load(struct amiget_config *cfg);

#endif /* AMIGET_CONFIG_H */
