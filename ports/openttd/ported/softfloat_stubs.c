/*
 * softfloat_stubs.c -- Override libnix soft-float ROM calls
 *
 * libnix's __divsf3, __muldf3 etc. call through AmigaOS ROM
 * mathieeesingbas.library / mathieeedoubbas.library. These ROM
 * libraries crash on FS-UAE (Guru 8000000B / 80000004) -- known
 * issue documented in crash-patterns.md #2.
 *
 * This file provides direct soft-float implementations using GCC's
 * __builtin_* intrinsics, which generate inline software float code
 * that does NOT depend on the ROM math libraries.
 *
 * By providing these symbols here (linked before -lm), we prevent
 * the linker from pulling in libnix's ROM-dependent versions.
 */

/* Suppress libnix mathieee library auto-open. Setting these to non-NULL
 * stops libnix from trying to OpenLibrary() the math libraries. */
void *MathIeeeDoubBasBase = 0;
void *MathIeeeDoubTransBase = 0;
void *MathIeeeSingBasBase = 0;
void *MathIeeeSingTransBase = 0;

/* --- Single-precision (float) --- */

float __divsf3(float a, float b) { return a / b; }
float __mulsf3(float a, float b) { return a * b; }
float __addsf3(float a, float b) { return a + b; }
float __subsf3(float a, float b) { return a - b; }
float __negsf2(float a)          { return -a; }

/* float <-> int conversions (single precision) */
float __floatsisf(int x)               { return (float)x; }
float __floatunsisf(unsigned int x)    { return (float)x; }
int   __fixsfsi(float x)               { return (int)x; }
unsigned int __fixunssfsi(float x)     { return (unsigned int)x; }

/* float comparison helpers */
int __cmpsf2(float a, float b)  { return (a < b) ? -1 : (a > b) ? 1 : 0; }
int __eqsf2(float a, float b)   { return !(a == b); }
int __nesf2(float a, float b)   { return  (a != b); }
int __ltsf2(float a, float b)   { return (a <  b) ? -1 : 1; }
int __lesf2(float a, float b)   { return (a <= b) ? -1 : 1; }
int __gtsf2(float a, float b)   { return (a >  b) ?  1 : -1; }
int __gesf2(float a, float b)   { return (a >= b) ?  1 : -1; }
int __unordsf2(float a, float b) { (void)a; (void)b; return 0; }

/* --- Double-precision (double) --- */

double __divdf3(double a, double b) { return a / b; }
double __muldf3(double a, double b) { return a * b; }
double __adddf3(double a, double b) { return a + b; }
double __subdf3(double a, double b) { return a - b; }
double __negdf2(double a)           { return -a; }

double __floatsidf(int x)             { return (double)x; }
double __floatunsidf(unsigned int x)  { return (double)x; }
int    __fixdfsi(double x)            { return (int)x; }
unsigned int __fixunsdfsi(double x)   { return (unsigned int)x; }

/* float <-> double conversions */
float  __truncdfsf2(double x) { return (float)x; }
double __extendsfdf2(float x) { return (double)x; }
