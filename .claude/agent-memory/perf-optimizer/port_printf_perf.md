---
name: port_printf_perf
description: Performance findings for ports/printf 1.28 — strchr() in format scan loop, no file I/O hotpath, single-shot tool reviewed 2026-03-26
type: project
---

# Performance Findings: ports/printf 1.28

**Reviewed:** 2026-03-26

## Hot Paths

1. **main() format scan loop (printf.c:136-249)** — Iterates the format string character by character. For each `%` conversion: two `strchr()` calls to skip flag chars and digits.
2. **print_escape_str() (printf.c:261-295)** — Called for `%b` arguments; walks the string with putchar() per character.

## Key Findings

### MEDIUM: `strchr(SKIP2, *fmt)` in inner format loop (printf.c:163, 173)

```c
for (; strchr(SKIP2, *fmt); ++fmt)
    ;
```
Where `SKIP2 = "0123456789"`. Called twice per `%` conversion specifier: once to skip to precision, once inside the precision block. `strchr()` scans the 10-character string for each format character. Replace with `isdigit()` which is a direct table lookup (1-2 cycles on 68000 via ctype table) vs strchr's linear scan (~15-20 cycles for SKIP2).

**Fix:**
```c
for (; isdigit((unsigned char)*fmt); ++fmt)
    ;
```
This also correctly handles `*fmt == '\0'` (isdigit returns false for NUL).

### LOW: `strchr(SKIP1, *fmt)` for flag characters (printf.c:153)

```c
for (; strchr(SKIP1, *fmt); ++fmt)
    ;
```
Where `SKIP1 = "#-+ 0"` (5 chars). Similar pattern. Could be replaced with a 5-case switch or a direct character test. Impact is lower because SKIP1 is shorter and flag sequences are typically 0-2 chars.

### LOW: putchar() per character in print_escape_str() (printf.c:289)

The `%b` escape string processor calls `putchar(*str)` for every literal character. For short escape strings (typical), this is negligible. For a `printf "%b" "long_string"` call, `fwrite()` for literal runs would be faster. In practice `printf` is invoked once with a short format string, so this is marginal.

### NO ISSUES: No file I/O loops

printf is a single-shot argument processor. There are no file reading loops, no large buffers. The `mklong()` static buffer uses `realloc` correctly with a size cache.

### NO ISSUES: Stack size

`__stack = 8192` is adequate. No large local arrays.

## Summary

- **Primary bottleneck:** Startup overhead (argv expansion, option parsing). Format processing is fast for typical one-shot invocations.
- **Estimated overall impact:** Marginal -- printf is invoked once per shell command, not in tight loops. The strchr->isdigit fix is clean but saves microseconds, not milliseconds.
- **No critical performance issues found.**
