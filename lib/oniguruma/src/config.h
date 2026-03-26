/*
 * config.h -- Oniguruma configuration for AmigaOS 68k (amiport)
 *
 * Minimal config for ASCII-only build with libnix (-noixemul).
 * No threads, no Unicode property tables, no JIT.
 */

#ifndef ONIG_CONFIG_H
#define ONIG_CONFIG_H

#define HAVE_SYS_TYPES_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define STDC_HEADERS 1

/* 68k sizes */
#define SIZEOF_INT 4
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_VOIDP 4

/* No stdint.h in C89 libnix */
/* #undef HAVE_STDINT_H */
/* #undef HAVE_INTTYPES_H */
/* #undef HAVE_UNISTD_H */

#define ONIG_EXTERN extern
#define PACKAGE "oniguruma"
#define VERSION "6.9.9"

/* Disable GCC computed goto -- may not work correctly on 68000 */
#define ONIG_DISABLE_DIRECT_THREADING 1

#endif /* ONIG_CONFIG_H */
