---
name: jq_1.7.1_portability_analysis
description: Key findings from portability analysis of jq 1.7.1 — blockers, pthread TLS design, fdopen crash, generated files needed
type: project
---

# jq 1.7.1 — Source Analyzer Findings

**Why:** Complex multi-file C99 project requiring structural rewrites before any build attempt succeeds.

**How to apply:** Code-transformer must implement these in order — skip any one and the build will fail or crash at runtime.

## Two Non-Negotiable Blockers

1. `-std=c99` in CFLAGS — 60+ for-init declaration errors. Must be first flag added.
2. pthread TLS rewrite — jv_thread.h, jv.c, jv_alloc.c, jv_dtoa_tsd.c all use pthread_key/once/mutex. Create `jv_thread_amiga.h` with inline static stubs. Only 3 keys ever created at runtime. Static array of 8 values is sufficient.

## One Crash-Guaranteed Item (crash-patterns #12)

`jv_file.c:14-26` calls `open()` then `fdopen()` — different fd namespaces. Replace with `fopen()` + separate `stat()` directory check. ~20 line rewrite.

## Missing Autotools-Generated Files

- `src/version.h` — `#define JQ_VERSION "1.7.1"`
- `src/config_opts.inc` — build configuration string static
- `config.h` — all HAVE_* flags (see full report for complete list)

## Key HAVE_* Decisions

- HAVE_PTHREAD_KEY_CREATE=0, HAVE___THREAD=0 → routes to plain global static fallback
- HAVE_GMTIME_R=0, HAVE_LOCALTIME_R=0, HAVE_TIMEGM=0 → existing fallbacks compile clean
- HAVE_STRPTIME=0 → uses NetBSD implementation already in util.c
- HAVE_SETLOCALE=0 → guard already present in main.c
- HAVE_LIBONIG=0 → stubs out regex builtins
- USE_DECNUM=1, DECUSE64=0 → keep decimal support, avoid 64-bit arithmetic
- IEEE_MC68k → define this for jv_dtoa.c (m68k is big-endian IEEE-754)

## AmigaOS Path Handling

linker.c `path_is_relative()` only checks leading `/`. On AmigaOS paths use volume: syntax. Add #ifdef __AMIGA__ arm. Not crash-critical but affects module loading (-L flag).

## environ External

compile.c and builtin.c use `extern char **environ`. libnix may provide empty environ. Both callers guard `if (environ == NULL)` so no crash, but $ENV and env builtins return empty objects. Document in PORT.md.

## Stack Size

65536 bytes minimum — jq parses arbitrarily nested JSON recursively.

## Port Category

Category 2 (Scripting Interpreter). Test via vamos. No ncurses, no sockets.

## Effort Estimate

3-5 working days. Pthread TLS rewrite is the most complex part.
