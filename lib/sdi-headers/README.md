# lib/sdi-headers

System Development Includes (SDI) — vendored Aminet header set used by
NetSurf-MUI and many other AmigaOS application frontends that need to
write portable hook / library / dispatcher code across AmigaOS 3, OS4,
and MorphOS.

## Source

- Upstream: https://aminet.net/dev/c/SDI_headers.lha
- Version: 1.7 (2015-08-04)
- Authors: Jens Maus + Dirk Stoecker
- License: **PD (Public Domain)** -- per individual header `Distribution: PD`
  fields. Free to vendor and redistribute.
- Project page: https://github.com/adtools/SDI

## Headers

6 single-purpose macro/define headers:

| Header | Purpose |
|---|---|
| `SDI_compiler.h` | compiler-specific REG / SAVEDS / VARARGS / hot-attribute defines |
| `SDI_hook.h` | uniform `MakeHook` / `HOOKPROTO` macros across compilers |
| `SDI_interrupt.h` | `MakeInterrupt` macro across compilers |
| `SDI_lib.h` | library function entry-point macros |
| `SDI_misc.h` | misc compatibility shims |
| `SDI_stdarg.h` | varargs hooks for tag-list functions |

## Build

No build needed -- headers-only set. Consumers add
`-I/work/lib/sdi-headers/include` to CFLAGS (the
`ports/netsurf/Makefile` does this for the NetSurf-MUI build via the
docker run env).

## Used by

- `ports/netsurf/` -- NetSurf-MUI 3.11 frontend uses SDI heavily for
  cross-platform MUI hook / dispatcher code (`frontends/mui/include/macros/vapor.h`
  is the entry point that includes `SDI_compiler.h`).
- (potentially) future AmigaOS application ports that need hook /
  varargs portability across OS3/OS4/MorphOS.

## Vendored 2026-05-02

Discovered as a Stage 11 blocker in NetSurf-MUI compilation
(`frontends/mui/include/macros/vapor.h:22:10: fatal error:
SDI_compiler.h: No such file or directory`). Vendored to clear the
blocker; build advances through the MUI frontend Prefs files until
the next blocker (CyberGraphX headers, see
`project_netsurf_stage11_progress.md` addendum).
