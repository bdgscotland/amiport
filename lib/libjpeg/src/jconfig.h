/*
 * jconfig.h --- amiport build configuration for libjpeg 9f.
 *
 * Generated from jconfig.cfg with explicit values for bebbo-gcc 13.3 +
 * libnix `-noixemul -m68040 -m68881`. See jconfig.txt for the meaning
 * of each define.
 *
 * KEY AMIPORT CHOICE: DCT_FLOAT_SUPPORTED is NOT defined.
 *
 * Why: the floating-point DCT/IDCT path uses double precision math.
 * With our `-m68881` CPU target, GCC inlines FPU instructions for
 * these -- which works on Vampire / 68040 / 68060 hardware. BUT:
 *   - On 68000-only configurations, double math would pull libm
 *     symbols routing through ROM mathieee*.library (FS-UAE crash
 *     family, crash-patterns #2 variant).
 *   - On FS-UAE 68882 emulation, transcendental FPU instructions
 *     trigger the FS-UAE 68882 transcendental gap (per the existing
 *     SDL2/Apollo pitfalls).
 *
 * The integer DCT methods (JDCT_ISLOW + JDCT_IFAST) provide
 * equivalent functionality and are the default for libjpeg's typical
 * web-image decode workload. NetSurf's JPEG decoder does NOT request
 * the float method, so this exclusion is invisible to consumers.
 */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* `void` and `const` are already correctly defined by GCC; do NOT
 * #define them away. */
/* CHAR_IS_UNSIGNED -- not relevant to bebbo-gcc */
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#define HAVE_LOCALE_H
/* NEED_BSD_STRINGS -- libnix has memcpy/memset; don't need BSD bcopy */
/* NEED_SYS_TYPES_H -- libnix sys/types.h; included via jinclude.h */
/* NEED_FAR_POINTERS -- 16-bit DOS only, not 68k */
/* NEED_SHORT_EXTERNAL_NAMES -- 6-char linker, not 68k */
/* INCOMPLETE_TYPES_BROKEN -- modern compiler, leave undef */

#ifdef JPEG_INTERNALS

/* RIGHT_SHIFT_IS_UNSIGNED: defined IF (signed_int >> n) is logical
 * (unsigned) rather than arithmetic (signed). bebbo-gcc on m68k uses
 * arithmetic right shift for signed types, so leave undef. */

/* INLINE: GCC supports `__inline__`. */
#define INLINE __inline__

/* DEFAULT_MAX_MEM: max amount of memory for the JPEG memory manager
 * to use before spilling to a backing file. We use jmemnobs.c which
 * has no backing store, so this is effectively the max image memory.
 * Set to 16 MB -- plenty for typical web JPEGs (rare to decode >2 MB
 * compressed at once). */
#define DEFAULT_MAX_MEM (16L * 1024L * 1024L)

/* NO_MKTEMP: we use jmemnobs.c which doesn't need mktemp. Define so
 * jmemmgr.c doesn't try to use it. */
#define NO_MKTEMP

#endif /* JPEG_INTERNALS */

/*
 * The JPEG_CJPEG_DJPEG block is for the cjpeg/djpeg command-line
 * tools, which we don't build as part of the library. The defines
 * are harmless if left out.
 */

/*
 * AMIPORT EXCLUSION: do NOT define DCT_FLOAT_SUPPORTED.
 *
 * jmorecfg.h normally enables this, but we want it disabled. The
 * cleanest way is to set the JPEG_NO_FLOAT_DCT macro that... actually
 * jmorecfg.h #defines DCT_*_SUPPORTED unconditionally. We need to
 * UNDEFINE DCT_FLOAT_SUPPORTED after including jmorecfg.h.
 *
 * The jpeglib.h include order is:
 *   1. jconfig.h (this file)
 *   2. jmorecfg.h (sets DCT_FLOAT_SUPPORTED)
 *   3. (rest of jpeglib.h)
 *
 * So we can't undefine it from jconfig.h. Instead we use the
 * compiler's `-UDCT_FLOAT_SUPPORTED` flag in the Makefile to
 * un-define it AFTER jmorecfg.h has been preprocessed... no, that
 * doesn't work either (compiler -U is processed BEFORE compilation).
 *
 * The correct fix: patch jmorecfg.h in src/ to gate the define on a
 * negation flag (`#ifndef JPEG_AMIPORT_NO_FLOAT_DCT`) -- see the
 * patch at src/jmorecfg.h.
 */
