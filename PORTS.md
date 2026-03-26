# amiport — Ported Software Catalog

Programs ported to AmigaOS 3.x using the amiport pipeline.

Version format: `upstream[-portrev]` — port revision is shown when > 1 (e.g., `1.68-2` = upstream 1.68, second port revision). Revision 1 is implicit.

| Port | Version | Description | Category | Original | Status | Aminet |
|------|---------|-------------|----------|----------|--------|--------|
| [cal](ports/cal/) | 1.32 | Unix-style calendar display | CLI | OpenBSD cal v1.32 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [cut](ports/cut/) | 1.28 | Extract fields/columns from text | CLI | OpenBSD cut v1.28 (BSD 3-Clause) | Built & tested | Submitted 2026-03-20 |
| [diff](ports/diff/) | 1.95 | File comparison utility | CLI | OpenBSD diff v1.95 (ISC + BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [grep](ports/grep/) | 1.68 | Pattern search (regex/fixed string) | CLI | OpenBSD grep v1.68 (BSD 2-Clause) | Built & tested | Submitted 2026-03-22 |
| [sed](ports/sed/) | 1.47 | Stream editor (text transformation) | CLI | OpenBSD sed v1.47 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [lua](ports/lua/) | 5.4.7 | Lua scripting language | Scripting | PUC-Rio Lua 5.4.7 (MIT) | Built & tested (65/65 FS-UAE) | Submitted 2026-03-22 |
| [tail](ports/tail/) | 1.24 | Display last part of a file | CLI | OpenBSD tail v1.24 (BSD 3-Clause) | Built & tested | Not submitted |
| [tee](ports/tee/) | 1.15 | Duplicate standard input | CLI | OpenBSD tee v1.15 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [head](ports/head/) | 1.24 | Print first lines of files | CLI | OpenBSD head v1.24 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [wc](ports/wc/) | 1.32 | Count lines, words, and characters | CLI | OpenBSD wc v1.32 (BSD 3-Clause) | Built & tested | Not submitted |
| [sort](ports/sort/) | 1.0 | Sort lines of text files | CLI | Plan 9 from User Space (Lucent Public License 1.02) | Built & tested | Not submitted |
| [uniq](ports/uniq/) | 1.33 | Filter/report repeated lines in files | CLI | OpenBSD uniq v1.33 (BSD 3-Clause) | Built & tested (29/29 FS-UAE) | Not submitted |
| [yes](ports/yes/) | 1.9 | Repeatedly output a string | CLI | OpenBSD yes v1.9 (BSD 3-Clause) | Built & tested | Submitted 2026-03-22 |
| [jq](ports/jq/) | 1.7.1-2 | Lightweight command-line JSON processor | CLI | jqlang jq 1.7.1 (MIT) + Oniguruma 6.9.9 (BSD) | Built & tested (60/60 FS-UAE) | Submitted 2026-03-23 |
| [bc](ports/bc/) | 1.07.1 | Arbitrary precision calculator | CLI | GNU bc 1.07.1 (GPL-3.0) | Built & tested (37/37 FS-UAE) | Submitted 2026-03-23 |
| [patch](ports/patch/) | 1.78 | Apply diff patches to source files | CLI | OpenBSD patch v1.78 (ISC + BSD) | Built & tested (42/42 FS-UAE) | Not submitted |
| [less](ports/less/) | 692 | Terminal pager with search and scroll | Console UI | GNU less 692 (GPL-3.0 / Less License) | Built & tested (20/20 FS-UAE) | Not submitted |
| [mg](ports/mg/) | 3.7 | Micro Emacs-like text editor | Console UI | troglobit/mg 3.7 (Public Domain) | Built & tested (18/18 FS-UAE) | Not submitted |
| [comm](ports/comm/) | 1.11 | Compare two sorted files line by line | CLI | OpenBSD comm v1.11 (BSD 3-Clause) | Built & tested (30/30 FS-UAE) | Not submitted |
| [rev](ports/rev/) | 1.16 | Reverse lines character by character | CLI | OpenBSD rev v1.16 (BSD 3-Clause) | Built & tested (24/24 FS-UAE) | Not submitted |
| [expand](ports/expand/) | 1.15 | Convert tabs to spaces | CLI | OpenBSD expand v1.15 (BSD 3-Clause) | Built & tested (29/29 FS-UAE) | Not submitted |
| [awk](ports/awk/) | 2024.12.25 | Pattern scanning and processing language | Scripting | BWK "One True Awk" (MIT-like) | Built & tested (44/44 FS-UAE) | Not submitted |
| [basename](ports/basename/) | 1.14 | Strip directory and suffix from filenames | CLI | OpenBSD basename v1.14 (BSD 3-Clause) | Built & tested (32/32 FS-UAE) | Not submitted |
| [dirname](ports/dirname/) | 1.17 | Strip last component from filename | CLI | OpenBSD dirname v1.17 (ISC) | Built & tested (24/24 FS-UAE) | Not submitted |
| [true](ports/true/) | 1.1 | Return true value (exit 0) | CLI | OpenBSD true v1.1 (BSD 3-Clause) | Built & tested (13/13 FS-UAE) | Not submitted |
| [false](ports/false/) | 1.1 | Return false value (exit error) | CLI | OpenBSD false v1.1 (BSD 3-Clause) | Built & tested (14/14 FS-UAE) | Not submitted |
| [colrm](ports/colrm/) | 1.14 | Remove columns from lines | CLI | OpenBSD colrm v1.14 (BSD 3-Clause) | Built & tested (28/28 FS-UAE) | Not submitted |
| [factor](ports/factor/) | 1.30 | Factor a number into primes | Misc | OpenBSD factor v1.30 (BSD 3-Clause) | Built & tested (39/39 FS-UAE) | Not submitted |
| [getopt](ports/getopt/) | 1.10 | Parse command options in scripts | CLI | OpenBSD getopt v1.10 (Public Domain) | Built & tested (40/40 FS-UAE) | Not submitted |
| [jot](ports/jot/) | 1.56 | Print sequential or random data | CLI | OpenBSD jot v1.56 (BSD 3-Clause) | Built & tested (41/41 FS-UAE) | Not submitted |
| [unexpand](ports/unexpand/) | 1.13 | Convert spaces to tabs | CLI | OpenBSD unexpand v1.13 (BSD 3-Clause) | Built & tested (32/32 FS-UAE) | Not submitted |
| [cksum](ports/cksum/) | 1.0 | Display file checksums and sizes | CLI | FreeBSD cksum v1.0 (BSD 3-Clause) | Built & tested (37/37 FS-UAE) | Not submitted |
| [col](ports/col/) | 1.20 | Filter reverse line feeds | CLI | OpenBSD col v1.20 (BSD 3-Clause) | Built & tested (29/29 FS-UAE) | Not submitted |

## Aminet Publication Tracking

| Port | Submitted | Aminet URL | Last Checked |
|------|-----------|-----------|-------------|
| cal | 2026-03-20 | [util/cli/cal-1.0](https://aminet.net/package/util/cli/cal-1.0) | 2026-03-22 — Live, 72 downloads, uploaded as 1.0 (now 1.32 locally) |
| cut | 2026-03-20 | [util/cli/cut-1.0](https://aminet.net/package/util/cli/cut-1.0) | 2026-03-22 — Live, 60 downloads, uploaded as 1.0 (now 1.28 locally) |
| diff | 2026-03-22 | pending moderation | 2026-03-22 |
| grep | 2026-03-22 | [util/cli/grep-1.68](https://aminet.net/package/util/cli/grep-1.68) | 2026-03-22 — Live, 6 downloads |
| sed | 2026-03-22 | [util/cli/sed-1.47](https://aminet.net/package/util/cli/sed-1.47) | 2026-03-22 — Live, 4 downloads |
| lua | 2026-03-22 | pending moderation | 2026-03-22 — dev/lang category |
| head | 2026-03-22 | [util/cli/head-1.24](https://aminet.net/package/util/cli/head-1.24) | 2026-03-22 — Live |
| tail | — | — | — |
| tee | 2026-03-22 | [util/cli/tee-1.15](https://aminet.net/package/util/cli/tee-1.15) | 2026-03-22 — Live |
| wc | — | — | — |
| sort | — | — | — |
| yes | 2026-03-22 | [util/cli/yes-1.9](https://aminet.net/package/util/cli/yes-1.9) | 2026-03-22 — Live |
| jq | 2026-03-23 | pending moderation | 2026-03-23 |
| bc | 2026-03-23 | pending moderation | 2026-03-23 |

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
| patch | `stat/fstat`, `open/read/write/close/lseek`, `unlink/rename/mkdir/chdir/chmod`, `signal`, `getline/mkstemp`, `getopt_long`, `recallocarray`, `opendir/readdir/closedir`, `strtonum/warnc`, `emu_mmap/emu_munmap` |
| bc | `signal`, `isatty`, `getenv`, `check_break` |
| jq | `stat/fstat`, `isatty`, `realpath`, `gettimeofday` |
| comm | `err`, `check_break` |
| rev | `err/warn`, `getline` |
| expand | `err/errx`, `expand_argv/free_argv` |
| true | (none -- minimal binary) |
| false | (none -- minimal binary) |
| colrm | `err/errx`, `expand_argv/free_argv` |
| factor | `err/errx`, `expand_argv/free_argv` |
| getopt | `getopt`, `err`, `expand_argv/free_argv` |
| jot | `getopt`, `err/errx`, `expand_argv/free_argv` |
| unexpand | `err/errx`, `expand_argv/free_argv` |
| cksum | `err/errx/warn`, `expand_argv/free_argv` |
| col | `err/errx/warn`, `expand_argv/free_argv` |

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
