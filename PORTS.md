# amiport — Ported Software Catalog

Programs ported to AmigaOS 3.x using the amiport pipeline.

| Port | Version | Description | Category | Original | Status | Aminet |
|------|---------|-------------|----------|----------|--------|--------|
| [cal](ports/cal/) | 1.32 | Unix-style calendar display | CLI | OpenBSD cal v1.32 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [cut](ports/cut/) | 1.28 | Extract fields/columns from text | CLI | OpenBSD cut v1.28 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [diff](ports/diff/) | 1.95 | File comparison utility | CLI | OpenBSD diff v1.95 (ISC + BSD 3-Clause) | Built & tested | Not submitted |
| [grep](ports/grep/) | 1.68 | Pattern search (regex/fixed string) | CLI | OpenBSD grep v1.68 (BSD 2-Clause) | Built & tested | Not submitted |
| [sed](ports/sed/) | 1.47 | Stream editor (text transformation) | CLI | OpenBSD sed v1.47 (BSD 3-Clause) | Built & tested | Not submitted |
| [lua](ports/lua/) | 5.4.7 | Lua scripting language | Scripting | PUC-Rio Lua 5.4.7 (MIT) | Built & tested | Not submitted |

## Aminet Publication Tracking

| Port | Submitted | Aminet URL | Last Checked |
|------|-----------|-----------|-------------|
| cal | 2026-03-20 | Pending review | 2026-03-20 |
| cut | 2026-03-20 | Pending review | 2026-03-20 |
| diff | — | — | — |
| grep | — | — | — |
| sed | — | — | — |
| lua | — | — | — |

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
| lua | `mkstemp`, `tmpfile` |

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
