# Amiga C Coding Standards

These rules apply to ALL C code targeting AmigaOS in this project.

## Language Standard

- **ANSI C89** only. No C99 features unless targeting OS 4.x.

## Headers

- Use `<proto/*.h>` for Amiga system calls (never `<clib/*.h>` pragmas).

## Types

- Use Amiga types (`LONG`, `ULONG`, `STRPTR`, `BPTR`, `APTR`) when interfacing with OS libraries.
- `LONG` is `long` (32-bit) — use `%ld`/`%lu` format specifiers, not `%d`/`%u`.

## Exit Codes — CRITICAL

POSIX `exit(1)` is **invisible** to Amiga shells. Amiga scripts test with `IF WARN` (>=5), `IF ERROR` (>=10), `IF FAIL` (>=20).

- `exit(0)` — OK (RETURN_OK)
- `exit(5)` — warning (RETURN_WARN)
- `exit(10)` — error (RETURN_ERROR) — use instead of `exit(1)` / `exit(EXIT_FAILURE)`
- `exit(20)` — fatal (RETURN_FAIL)
- Fix `err(1, ...)` / `errx(1, ...)` calls too — change to `err(10, ...)`

## Version String

Every program must include a version string using the **upstream version** (not a generic `1.0`):
```c
static const char *verstag = "$VER: progname X.Y (DD.MM.YYYY)";
```
Use the upstream project's version number (e.g., `1.68` for OpenBSD grep rev 1.68, `5.4.7` for Lua). The VERSION in the Makefile, the $VER string, and the .readme must all match.

## Stack Size

Amiga default stack is 4KB. Always add a stack cookie:
```c
long __stack = 32768;  /* or 65536 for recursive programs */
```

## Temp Files

No `/tmp` on Amiga. Use `T:` (maps to `RAM:T/`). Use `amiport_tmpfile()` / `amiport_mkstemp()`.

## Epoch

AmigaOS epoch is 1978-01-01, Unix is 1970-01-01. Offset: 252460800 seconds (AMIGA_EPOCH_OFFSET).

## Transformation Comments

Document every POSIX-to-Amiga transformation:
```c
/* amiport: replaced POSIX open() with amiport_open() */
```

## Cross-Platform

Use `#ifdef __AMIGA__` blocks when code should remain cross-platform. Target AmigaOS 3.x on 68020+ as default.
