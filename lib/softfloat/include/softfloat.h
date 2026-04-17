/*
 * libsoftfloat -- Pure-integer IEEE 754 software float implementations
 *                 for AmigaOS 3.x with libnix on FS-UAE
 *
 * Why this exists: libnix's __divsf3, __mulsf3, __addsf3, __subsf3 (and
 * the double-precision __divdf3 etc.) call through ROM mathieeesingbas.library
 * and mathieeedoubbas.library. FS-UAE's ROM-library emulation is broken --
 * those functions Guru Meditation #8000000B (Line F / coprocessor exception).
 *
 * This library provides software-only IEEE 754 implementations that DON'T
 * call into mathieee* libraries. Override happens at link time: linking
 * libsoftfloat.a BEFORE -lm causes our symbols to win over libnix's.
 *
 * Usage:
 *   make build-softfloat
 *   <link your binary>: ... -L../../lib/softfloat -lsoftfloat -lm ...
 *
 * Header inclusion is OPTIONAL -- the symbols (__divsf3 etc.) are referenced
 * by GCC's codegen, not by user source code. This header just documents
 * what's provided.
 *
 * Discovered via libSDL2-amigaos3 src/stdlib/SDL_os3float.c + SDL_os3double.c.
 * Promoted into amiport lib/ for cross-port reuse (PDR-015 OpenTTD work).
 */

#ifndef AMIPORT_SOFTFLOAT_H
#define AMIPORT_SOFTFLOAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Single-precision IEEE 754 binary32 */
float __divsf3(float a, float b);
float __mulsf3(float a, float b);
float __addsf3(float a, float b);
float __subsf3(float a, float b);
int __fixsfsi(float a);
float __floatsisf(int a);
int __cmpsf2(float a, float b);
int __eqsf2(float, float);
int __nesf2(float, float);
int __gtsf2(float, float);
int __gesf2(float, float);
int __ltsf2(float, float);
int __lesf2(float, float);

/* Double-precision IEEE 754 binary64 */
double __divdf3(double a, double b);
double __muldf3(double a, double b);
double __adddf3(double a, double b);
double __subdf3(double a, double b);
int __fixdfsi(double a);
double __floatsidf(int a);
int __cmpdf2(double a, double b);
int __eqdf2(double, double);
int __nedf2(double, double);
int __gtdf2(double, double);
int __gedf2(double, double);
int __ltdf2(double, double);
int __ledf2(double, double);

/* Cross-precision conversion */
double __extendsfdf2(float a);
float __truncdfsf2(double a);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_SOFTFLOAT_H */
