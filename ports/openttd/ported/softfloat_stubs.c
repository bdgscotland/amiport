/*
 * softfloat_stubs.c -- Override libnix soft-float ROM calls
 *
 * libnix's __divsf3, __muldf3 etc. call through AmigaOS ROM
 * mathieeesingbas.library / mathieeedoubbas.library. These ROM
 * libraries crash on FS-UAE (Guru 8000000B / 80000004).
 *
 * GCC has built-in soft-float that doesn't need ROM.
 * By providing these symbols here (linked before -lm), we
 * prevent the linker from pulling in libnix's ROM-dependent versions.
 *
 * These use GCC's __builtin_* intrinsics which generate inline
 * software float code.
 */

/* Suppress libnix mathieee library auto-open */
void *MathIeeeDoubBasBase = 0;
void *MathIeeeDoubTransBase = 0;
void *MathIeeeSingBasBase = 0;
void *MathIeeeSingTransBase = 0;
