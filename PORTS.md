# amiport — Ported Software Catalog

Programs ported to AmigaOS 3.x using the amiport pipeline.

| Port | Version | Description | Category | Original | Status | Aminet |
|------|---------|-------------|----------|----------|--------|--------|
| [cal](ports/cal/) | 1.32 | Unix-style calendar display | CLI | OpenBSD cal v1.32 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [cut](ports/cut/) | 1.28 | Extract fields/columns from text | CLI | OpenBSD cut v1.28 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [diff](ports/diff/) | 1.95 | File comparison utility | CLI | OpenBSD diff v1.95 (ISC + BSD 3-Clause) | Built & tested | Not submitted |
| [grep](ports/grep/) | 1.68 | Pattern search (regex/fixed string) | CLI | OpenBSD grep v1.68 (BSD 2-Clause) | Built & tested | Submitted 2026-03-22 |
| [sed](ports/sed/) | 1.47 | Stream editor (text transformation) | CLI | OpenBSD sed v1.47 (BSD 3-Clause) | Built & tested | Not submitted |
| [lua](ports/lua/) | 5.4.7 | Lua scripting language | Scripting | PUC-Rio Lua 5.4.7 (MIT) | Built & tested | Not submitted |
| [tail](ports/tail/) | 1.24 | Display last part of a file | CLI | OpenBSD tail v1.24 (BSD 3-Clause) | Built & tested | Not submitted |
| [tee](ports/tee/) | 1.15 | Duplicate standard input | CLI | OpenBSD tee v1.15 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [head](ports/head/) | 1.24 | Print first lines of files | CLI | OpenBSD head v1.24 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [wc](ports/wc/) | 1.32 | Count lines, words, and characters | CLI | OpenBSD wc v1.32 (BSD 3-Clause) | Built & tested | Not submitted |
| [sort](ports/sort/) | 1.0 | Sort lines of text files | CLI | Plan 9 from User Space (Lucent Public License 1.02) | Built & tested | Not submitted |
| [yes](ports/yes/) | 1.9 | Repeatedly output a string | CLI | OpenBSD yes v1.9 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |

## Aminet Publication Tracking

| Port | Submitted | Aminet URL | Last Checked |
|------|-----------|-----------|-------------|
| cal | 2026-03-20 | [util/cli/cal-1.0](https://aminet.net/package/util/cli/cal-1.0) | 2026-03-21 — Live, uploaded as 1.0 (now 1.32 locally) |
| cut | 2026-03-20 | [util/cli/cut-1.0](https://aminet.net/package/util/cli/cut-1.0) | 2026-03-21 — Live, 29 downloads, uploaded as 1.0 (now 1.28 locally) |
| diff | — | — | — |
| grep | 2026-03-22 | util/cli/grep-1.68 (pending moderation) | — |
| sed | — | — | — |
| lua | — | — | — |
| head | 2026-03-22 | util/cli/head-1.24 (pending moderation) | — |
| tail | — | — | — |
| tee | 2026-03-22 | util/cli/tee-1.15 (pending moderation) | — |
| wc | — | — | — |
| sort | — | — | — |
| yes | 2026-03-22 | util/cli/yes-1.9 (pending moderation) | — |

Run `make check-aminet` to verify publication status.

## Shim Functions Coverage

Each port exercises different parts of the posix-shim library:

| Port | Shim Functions Used |
|------|-------------------|
| cal | `getopt`, `err/errx`, `strtonum` |
| cut | `getopt`, `err/errx/warn`, `strtonum` |
| diff | `getopt`, `err/errx/warn/warnc`, `stat`, `opendir/readdir/closedir`, `scandir/alphasort`, `fnmatch`, `tmpfile` |
| grep | `getopt`, `err/errx/warn/warnc`, `strtonum`, `reallocarray`, `opendir/readdir/closedir`, `isatty`, `regex` (Tier 2 emu) |
| sed | `getopt`, `err/errx/warn/errc`, `strtonum`, `strlcpy/strlcat`, `reallocarray`, `mkstemp`, `regex` (Tier 2 emu) |
| lua | `tmpfile` |
| tail | `getopt`, `err/errx/warn/warnx`, `stat/fstat`, `lseek`, `write`, `strlcpy`, `reallocarray`, `recallocarray`, `fpurge`, `check_break` |
| tee | `open`, `read`, `write`, `close`, `signal`, `check_break`, `getopt`, `expand_argv`, `err/warn` |
| head | `getopt`, `err/errx/warn`, `strtonum`, `expand_argv`, `free_argv` |
| wc | `open`, `read`, `close`, `fstat`, `getopt`, `err/warn`, `expand_argv`, `free_argv` |
| sort | `fopen`, `signal`, `check_break`, `getpid` |
| yes | `err`, `check_break` |

## How to Build

```bash
make build TARGET=ports/cal     # Build a specific port
make build-ports                # Build all ports
make test TARGET=ports/cal      # Test in vamos
make list-ports                 # Show status of all ports
```

## How to Add a Port

Use the `/port-project` skill — it handles the full pipeline:
```
/port-project /path/to/source
```

See [docs/porting-guide.md](docs/porting-guide.md) for the manual process.
