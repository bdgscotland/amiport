/* amiport/compat.h — 68k Platform Compatibility
 *
 * Fixes for correct C code that breaks on Motorola 68k / bebbo-gcc.
 * These are NOT POSIX shims — they fix compiler and hardware assumptions
 * that differ from modern platforms.
 *
 * Include early in any port with custom allocators, large struct returns,
 * or alignment-sensitive code.
 */

#ifndef AMIPORT_COMPAT_H
#define AMIPORT_COMPAT_H

#ifdef __AMIGA__

/*
 * 68k Alignment Fix
 *
 * On 68000, offsetof(struct { char x; union { int; double; } u; }, u)
 * returns 2 (word alignment). On x86/ARM it returns 4 or 8.
 *
 * Code that uses offsetof to calculate allocator alignment will pack
 * blocks at 2-byte boundaries. Storing int/long/pointer values at
 * 2-byte-aligned addresses works on 68020+ (unaligned access handled
 * in hardware) but reads corrupted data on 68000.
 *
 * Even on 68020+, some code depends on the alignment value being >= 4
 * for correct pointer arithmetic (e.g., storing a stack_ptr before
 * each allocated block).
 *
 * Use AMIPORT_ALIGN() around any offsetof-based alignment calculation
 * to enforce a minimum of 4 bytes.
 *
 * Discovered in jq 1.7.1 port: exec_stack.h ALIGNMENT=2 caused the
 * VM stack to corrupt jv struct kind_flags. See crash-patterns #13.
 */
#define AMIPORT_ALIGN(x) (((x) < 4) ? 4 : (x))

/*
 * 68k Byte Order
 *
 * Motorola 68k is big-endian (MSB at lowest address).
 * Use for IEEE floating-point configuration (dtoa, etc.)
 */
#define AMIPORT_BIG_ENDIAN 1

/*
 * Optimization Level Warning
 *
 * bebbo-gcc 6.5.0b generates incorrect code at -O1 and -O2 for
 * functions that return structs > 8 bytes by value, especially in
 * expression contexts like f(g(x)) where the return is immediately
 * consumed as an argument.
 *
 * Symptom: struct's first byte (e.g., type tag / kind field) reads
 * as 0 despite being set correctly inside the returning function.
 * The rest of the struct may contain valid-looking but shifted data.
 *
 * This CANNOT be fixed in a header — it requires -O0 in CFLAGS.
 * The source-analyzer flags this as a warning when it detects
 * struct-by-value returns > 8 bytes.
 *
 * Discovered in jq 1.7.1 port: jv struct (16 bytes) returned by
 * jv_string(), jv_array_get(), etc. See crash-patterns #14.
 */

/* No-op marker macro: annotate functions that return large structs.
 * Helps agents and reviewers identify affected functions. */
#define AMIPORT_LARGE_STRUCT_RETURN /* requires -O0 on bebbo-gcc */

#endif /* __AMIGA__ */
#endif /* AMIPORT_COMPAT_H */
