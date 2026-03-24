# Amiga Port Candidates — Hardware Inventory Catalog

Progressively populated catalog of software candidates for AmigaOS porting, organized by target hardware tier.

## Hardware Tiers

| Tier | Hardware | CPU | RAM | Key Additions | Target |
|------|----------|-----|-----|---------------|--------|
| **T1** | Stock A1200 | 68EC020 @ 14MHz | 2MB Chip | AGA, IDE | Primary — current focus |
| **T2** | A1200 + Fast RAM | 68EC020 @ 14MHz | 2MB Chip + 4MB Fast | 6MB total | Common upgrade |
| **T3** | A1200 + accelerator | 68030/040 @ 25-50MHz | 8-16MB Fast | FPU, MMU, speed | Enthusiast |
| **T4** | A4000/040 | 68040 @ 25MHz | 2MB Chip + 4-16MB Fast | Full 32-bit, FPU | High-end classic |
| **T5** | A1200/A4000 + 68060 | 68060 @ 50-75MHz | 16-128MB | Fast CPU, large RAM | Maxed classic |
| **T6** | A500/A600 | 68000 @ 7MHz | 512KB-2MB Chip | OCS/ECS, no IDE (A500) | Legacy — limited |

## Classification Key

**Status:**
- `PORTED` — Already in amiport, built & tested
- `EXISTS` — Already on Aminet/GG/other (details in Notes)
- `CANDIDATE` — Feasible, no existing port found
- `STRETCH` — Feasible but tight on target hardware (RAM/speed)
- `NEEDS_HIGHER_TIER` — Needs more capable hardware than listed tier
- `INFEASIBLE` — Cannot practically run on any classic Amiga
- `UPGRADE` — Exists but our port would add value (modern version, -noixemul, etc.)

**Category** (per ADR-011):
- Cat 1: CLI text filter (stdin/stdout)
- Cat 2: CLI with file I/O
- Cat 3: Console UI (ncurses/termcap)
- Cat 4: Network (BSD sockets)
- Cat 5: GUI (Intuition/MUI)

**Priority:** `HIGH` / `MEDIUM` / `LOW`

---

# Tier 1: Stock Amiga 1200

**68EC020 @ 14MHz, 2MB Chip RAM, AGA, Kickstart 3.1**

Constraints: ~1MB available after OS boot (~1.5MB without Workbench). No FPU. No MMU. Typical user has IDE hard drive.

## 1. Text Processing (CLI)

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| cal | OpenBSD 1.32 | 1 | PORTED | — | On Aminet |
| cut | OpenBSD 1.28 | 1 | PORTED | — | On Aminet |
| head | OpenBSD 1.24 | 1 | PORTED | — | On Aminet |
| tail | OpenBSD 1.24 | 1 | PORTED | — | |
| tee | OpenBSD 1.15 | 1 | PORTED | — | On Aminet |
| wc | OpenBSD 1.32 | 1 | PORTED | — | |
| yes | OpenBSD 1.9 | 1 | PORTED | — | On Aminet |
| uniq | OpenBSD 1.33 | 1 | PORTED | — | |
| sort | Plan 9 1.0 | 1 | PORTED | — | |
| grep | OpenBSD 1.68 | 2 | PORTED | — | On Aminet |
| sed | OpenBSD 1.47 | 2 | PORTED | — | On Aminet |
| diff | OpenBSD 1.95 | 2 | PORTED | — | |
| patch | OpenBSD 1.78 | 2 | PORTED | — | |
| tr | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG, jffabre. Multiple ports exist. |
| fold | OpenBSD | 1 | EXISTS | LOW | jffabre port supports Amiga wildcards. adtools/coreutils. |
| rev | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils (as tac). |
| expand | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG textutils. |
| unexpand | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG textutils. |
| paste | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG textutils. |
| join | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG textutils. |
| comm | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils, GG textutils. |
| fmt | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils. |
| nl | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils. |
| od | OpenBSD | 1 | EXISTS | LOW | adtools/coreutils. |
| split | OpenBSD | 2 | EXISTS | LOW | Aminet util/sys/split.lha + adtools/coreutils. |
| col | OpenBSD | 1 | VERIFY | LOW | Filter reverse line feeds. |
| column | OpenBSD | 1 | CANDIDATE | MEDIUM | Columnate lists. No existing port found. |
| hexdump | OpenBSD | 1 | CANDIDATE | MEDIUM | Hex dump with format control. BSD util, not in GNU coreutils. |
| csplit | OpenBSD | 2 | CANDIDATE | LOW | Split by context. May be in GG but unconfirmed. |
| strings | OpenBSD | 2 | CANDIDATE | HIGH | No standalone port found. Invaluable for Amiga devs. |
| look | OpenBSD | 1 | VERIFY | LOW | Binary search through sorted file. |
| tsort | OpenBSD | 1 | VERIFY | LOW | Topological sort. |
| vis/unvis | OpenBSD | 1 | VERIFY | LOW | Display non-printable characters. |

## 2. File & Directory Tools

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| find | OpenBSD / GNU | 2 | CANDIDATE | HIGH | No port found. Native SEARCH is not equivalent. Critical for scripting. |
| xargs | OpenBSD | 2 | CANDIDATE | HIGH | No port found. Essential for pipelines. Pairs with find. |
| cmp | OpenBSD | 2 | CANDIDATE | HIGH | No port found. Byte-level file comparison. Small, clean. |
| tree | Steve Baker | 2 | CANDIDATE | HIGH | No port found. Very popular utility. Native DIR ALL not equivalent. |
| touch | OpenBSD | 2 | CANDIDATE | MEDIUM | No port found. SETDATE not equivalent. Build systems need this. |
| file | Ian Darwin | 2 | CANDIDATE | MEDIUM | No port found. No AmigaOS equivalent for magic-number detection. |
| basename | OpenBSD | 1 | CANDIDATE | HIGH | No port found. Essential for shell scripts. Trivial to port. |
| dirname | OpenBSD | 1 | CANDIDATE | HIGH | No port found. Pairs with basename. Trivial to port. |
| du | OpenBSD | 2 | UPGRADE | MEDIUM | DiskUsage on Aminet + Mnemosyne exist but aren't du(1) compatible. |
| install | OpenBSD | 2 | VERIFY | LOW | Copy files with attributes. |
| readlink | OpenBSD | 2 | VERIFY | LOW | Amiga softlinks are limited. |
| stat | OpenBSD | 2 | VERIFY | LOW | Display file status. |
| mktemp | OpenBSD | 2 | VERIFY | LOW | Create temp file safely. |

## 3. Archivers & Compression

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| gzip/gunzip | GNU | 2 | EXISTS | — | On Aminet (gzip 1.2.4 etc). |
| bzip2 | Julian Seward | 2 | EXISTS | — | On Aminet. |
| tar | OpenBSD / GNU | 2 | EXISTS | — | GG tar exists. |
| unzip | Info-ZIP | 2 | EXISTS | — | On Aminet. |
| zip | Info-ZIP | 2 | EXISTS | — | On Aminet. |
| xz/lzma | Tukaani | 2 | VERIFY | HIGH | Modern compression. Dictionary may be tight on 2MB. |
| zstd | Facebook | 2 | STRETCH | MEDIUM | Default dict 128KB OK, but larger modes need Fast RAM. |
| cpio | OpenBSD | 2 | VERIFY | LOW | Archive format. Less common. |

## 4. Shell Utilities

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| echo | OpenBSD | 1 | EXISTS | — | Amiga has built-in Echo. |
| sleep | OpenBSD | 1 | EXISTS | — | Amiga has Wait. |
| env | OpenBSD | 1 | VERIFY | MEDIUM | Run with modified environment. |
| printf | OpenBSD | 1 | VERIFY | MEDIUM | Format and print data. |
| seq | Plan 9 / GNU | 1 | VERIFY | MEDIUM | Print number sequences. Useful for scripting. |
| date | OpenBSD | 1 | VERIFY | MEDIUM | Date formatting. Needs epoch conversion. |
| which | OpenBSD | 1 | VERIFY | MEDIUM | Locate a program. Needs PATH/C: awareness. |
| uname | — | 1 | CANDIDATE | LOW | System info. Amiga-specific implementation needed. |
| tty | OpenBSD | 1 | VERIFY | LOW | Print terminal name. |
| nohup | OpenBSD | 1 | VERIFY | LOW | Limited meaning on AmigaOS. |
| timeout | GNU | 1 | VERIFY | MEDIUM | Run command with time limit. |

## 5. Math & Calculation

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| bc | GNU 1.07.1 | 2 | PORTED | — | On Aminet (pending) |
| dc | OpenBSD | 1 | VERIFY | MEDIUM | RPN calculator. Often bundled with bc. |
| factor | OpenBSD | 1 | VERIFY | LOW | Prime factorization. |
| units | GNU | 1 | VERIFY | LOW | Unit conversion. Needs data file. |
| expr | OpenBSD | 1 | VERIFY | MEDIUM | Expression evaluator. |

## 6. Scripting & Languages

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| lua | PUC-Rio 5.4.7 | 2 | PORTED | — | On Aminet (pending) |
| jq | jqlang 1.7.1 | 2 | PORTED | — | On Aminet (pending) |
| awk | OpenBSD/one true awk | 2 | EXISTS | LOW | AT&T awk 1.0 on Aminet (1994, noixemul, SAS/C). Works. GG gawk also exists (ixemul). |
| m4 | GNU | 2 | CANDIDATE | LOW | May be in GG (ixemul). No standalone noixemul port. Dev tool only. |
| forth | various | 2 | VERIFY | MEDIUM | Small interpreters (pForth). Native Amiga Forths may exist. |
| scheme | various | 2 | VERIFY | MEDIUM | Small Schemes (chibi-scheme, s7). ~100KB. |

## 7. Console UI / Interactive (Category 3)

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| less | GNU 692 | 3 | PORTED | — | |
| mg | OpenBSD | 3 | UPGRADE | HIGH | MicroEMACS 3.10 (1989) on Aminet. Modern OpenBSD mg is vastly better. |
| nano | GNU | 3 | CANDIDATE | HIGH | No port exists. Very popular request. Needs console-shim ncurses. |
| nvi | OpenBSD | 3 | EXISTS | — | vim 6.0 (2002) on Aminet for 68k. No value over vim. Skip. |
| sc | n-t-roff/sc | 3 | CANDIDATE | MEDIUM | No port found. Classic spreadsheet. Needs ncurses. |
| ncdu | yorhel | 3 | CANDIDATE | MEDIUM | No port found. Use v1.x (C), not v2.x (Rust). Needs ncurses + dir walk. |
| indent | GNU 2.2.12 | 2 | CANDIDATE | LOW | MorphOS/PPC only on Aminet. No 68k port. Pure C, easy. |
| more | OpenBSD | 3 | EXISTS | — | less is superior; skip. |

## 8. Development Tools

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| ctags | Universal Ctags | 2 | UPGRADE | MEDIUM | Exuberant Ctags 5.5 (2003) on Aminet. Universal Ctags would be a big upgrade. |
| diff3 | OpenBSD | 2 | VERIFY | MEDIUM | Three-way diff. |
| sdiff | OpenBSD | 2 | VERIFY | MEDIUM | Side-by-side diff. |
| unifdef | — | 2 | VERIFY | MEDIUM | Remove #ifdef blocks. |
| cscope | Santa Cruz Op | 3 | VERIFY | MEDIUM | Source code browser. |
| cpp | — | 2 | EXISTS | — | Comes with GCC. |
| lint | — | 2 | VERIFY | LOW | C linter. |

## 9. Network Tools (Category 4)

Require bsdsocket.library (AmiTCP, Miami, Roadshow).

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| nc/netcat | OpenBSD | 4 | VERIFY | HIGH | Network Swiss army knife. Small. |
| curl | Daniel Stenberg | 4 | VERIFY | HIGH | HTTP client. Large but essential. |
| wget | GNU | 4 | VERIFY | MEDIUM | HTTP downloader. |
| whois | OpenBSD | 4 | VERIFY | MEDIUM | Domain lookup. Small. |
| ftp | OpenBSD | 4 | VERIFY | MEDIUM | FTP client. |
| telnet | OpenBSD | 4 | VERIFY | MEDIUM | Telnet client. |
| nslookup | ISC | 4 | VERIFY | MEDIUM | DNS lookup. |
| ping | OpenBSD | 4 | VERIFY | MEDIUM | ICMP ping. Needs raw sockets. |
| irc | various | 4 | VERIFY | MEDIUM | IRC client. Various small ones. |

## 10. Checksum & Crypto

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| md5 | OpenBSD | 1 | VERIFY | HIGH | MD5 hash. Small. Very useful. |
| sha1 | OpenBSD | 1 | VERIFY | HIGH | SHA-1 hash. Small. |
| sha256 | OpenBSD | 1 | VERIFY | MEDIUM | SHA-256. Slower on 68k but doable. |
| cksum | OpenBSD | 1 | VERIFY | MEDIUM | CRC checksum. |
| b64encode | OpenBSD | 1 | VERIFY | LOW | Base64 encode/decode. |

## 11. Miscellaneous

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| fortune | OpenBSD | 1 | VERIFY | MEDIUM | Random quotes. Needs data file. Fun. |
| figlet | Frank/Glenn/Ian | 1 | VERIFY | MEDIUM | ASCII art text. Needs font files. |
| cowsay | Tony Monroe | 1 | VERIFY | MEDIUM | Fun. Perl original, but C rewrites exist. |
| banner | OpenBSD | 1 | VERIFY | LOW | Print large letters. |
| number | OpenBSD | 1 | VERIFY | LOW | Convert numbers to English. |
| morse | OpenBSD | 1 | VERIFY | LOW | Morse code converter. |

## 12. Games & Interactive Fiction

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| frotz | Stefan Jokisch | 3 | VERIFY | HIGH | Z-machine interpreter. Plays all Infocom games. Small C, ncurses. |
| nethack | DevTeam | 3 | VERIFY | HIGH | Roguelike. Legendary. TTY mode fits console-shim. ~200KB. |
| angband | Ben Harrison | 3 | VERIFY | MEDIUM | Roguelike. Larger than nethack. May need Fast RAM. |
| rogue | Michael Toy | 3 | VERIFY | MEDIUM | Original roguelike. Very small. |
| 2048 | various | 3 | CANDIDATE | LOW | Terminal 2048 game. Trivial C implementations exist. |
| chess (gnuchess) | GNU | 2 | VERIFY | MEDIUM | CLI chess engine. CPU-bound but playable on 68020. |
| tetris | various | 3 | VERIFY | LOW | Terminal tetris. Many small C implementations. |

## 13. Data Format Tools

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| jq | jqlang 1.7.1 | 2 | PORTED | — | JSON processor. On Aminet (pending). |
| csvtool | various | 1 | VERIFY | MEDIUM | CSV processing. Several small C implementations. |
| xmlstarlet | Mikhail Grushinskiy | 2 | VERIFY | MEDIUM | XML processor (like jq for XML). Needs expat/libxml. |
| yq | various | 2 | VERIFY | LOW | YAML processor. Most are Go/Python — need C version. |
| base64 | OpenBSD | 1 | VERIFY | MEDIUM | Base64 encode/decode. |
| uuencode/uudecode | OpenBSD | 1 | VERIFY | MEDIUM | Classic encoding. Historically important for Amiga BBS era. |

## 14. Audio & Music Tools

The Amiga is legendary for audio. Tools that work with its native formats have special value.

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| sox | Chris Bagwell | 2 | VERIFY | HIGH | Swiss army knife of audio. Core features fit A1200, FPU helps. |
| mpg123 | Michael Hipp | 2 | VERIFY | MEDIUM | MP3 decoder. Very slow on 68020 without FPU. Tier 3+? |
| flac | Xiph.org | 2 | VERIFY | MEDIUM | FLAC decoder. Integer-only decode is feasible on 68020. |
| openmpt (libopenmpt) | OpenMPT | 2 | VERIFY | HIGH | MOD/S3M/XM/IT playback library. Direct Amiga relevance. |
| mid2agd | various | 2 | VERIFY | LOW | MIDI to Amiga format converters. Niche. |

## 15. Graphics & Image Tools

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| netpbm | various | 2 | VERIFY | HIGH | Image format conversion toolkit. Modular, small tools. PBM/PGM/PPM. |
| pngquant | Kornel Lesinski | 2 | VERIFY | MEDIUM | PNG compression/quantization. Useful for web prep. |
| gifski | Kornel Lesinski | 2 | VERIFY | LOW | GIF creation. Rust — needs C alternative. |
| jp2a | Christian Stigen Larsen | 1 | VERIFY | MEDIUM | Convert JPEG to ASCII art. Fun + useful. |
| caca-utils | Sam Hocevar | 2 | VERIFY | MEDIUM | Image to colored ASCII art (libcaca). |
| iff2png | various | 2 | VERIFY | HIGH | IFF/ILBM to PNG converter. Direct Amiga relevance. |
| png2iff | various | 2 | VERIFY | HIGH | PNG to IFF/ILBM converter. Direct Amiga relevance. |

## 16. Document & Text Rendering

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| groff | GNU | 2 | VERIFY | MEDIUM | Document formatter. Renders man pages. Large. |
| mandoc | OpenBSD | 2 | VERIFY | HIGH | Man page formatter. Much smaller than groff. Clean C. |
| discount | David Parsons | 2 | VERIFY | HIGH | Markdown to HTML. Small C library. Very useful. |
| lowdown | Kristaps Dzonsons | 2 | VERIFY | HIGH | Markdown renderer. Clean C, minimal deps. BSD licensed. |
| spell | OpenBSD | 2 | VERIFY | LOW | Spell checker. Needs dictionary file. |
| aspell | GNU | 2 | VERIFY | LOW | Spell checker. Larger than spell. |
| enscript | GNU | 2 | VERIFY | LOW | Text to PostScript. Niche on Amiga. |

## 17. System & Admin Tools

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| watch | Procps | 1 | VERIFY | MEDIUM | Run command repeatedly, show output. Needs Amiga timer. |
| time | OpenBSD | 1 | VERIFY | MEDIUM | Time command execution. ReadEClock-based on Amiga. |
| tput | ncurses | 1 | VERIFY | LOW | Terminal capability queries. Works with console-shim. |
| strace equiv | — | 2 | CANDIDATE | LOW | Would need SnoopDos-like approach. Amiga-specific. |
| lsof equiv | — | 2 | CANDIDATE | LOW | Show open files. Would use dos.library LockList. |

## 18. Embedded Databases & Libraries

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| sqlite | D. Richard Hipp | 2 | VERIFY | HIGH | Embedded SQL database. ~500KB. Needs Tier 3+ for real use. |
| gdbm | GNU | 2 | VERIFY | MEDIUM | Key-value database. Small. Used by many tools. |
| cdb | D.J. Bernstein | 2 | VERIFY | MEDIUM | Constant database. Very small, fast lookups. Perfect for A1200. |

---

# Tier 2: A1200 + Fast RAM (4-8MB)

**68EC020 @ 14MHz, 2MB Chip + 4-8MB Fast RAM, AGA**

Same CPU speed but ~6-10MB total RAM. Opens up larger programs.

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| tcl | Tcl Core Team | 2 | VERIFY | MEDIUM | ~300KB binary. Fits with 4MB Fast. |
| xz/lzma | Tukaani | 2 | VERIFY | HIGH | Compression with larger dictionary. |
| zstd | Facebook | 2 | VERIFY | MEDIUM | Full dictionary modes. |
| links | Twibright | 3+4 | VERIFY | HIGH | Text web browser. Needs ~2MB working set. |
| lynx | U of Kansas | 3+4 | VERIFY | MEDIUM | Text web browser. Larger than links. |
| ssh | OpenBSD | 4 | VERIFY | MEDIUM | Crypto is CPU-bound at 14MHz but functional. |
| emacs (mg) | OpenBSD | 3 | VERIFY | HIGH | If mg is too small, micro-emacs variants. |
| perl | Larry Wall | 2 | VERIFY | STRETCH | Old perl 4/5 might fit in 8MB. Slow at 14MHz. |

---

# Tier 3: A1200 + Accelerator (68030/040)

**68030 @ 25-50MHz or 68040 @ 25-40MHz, 8-32MB Fast RAM, FPU**

FPU enables floating-point programs. Speed + RAM enable larger tools.

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| gnuplot | Thomas Williams | 3 | VERIFY | HIGH | Plotting tool. Needs FPU for performance. |
| octave | GNU | 2 | VERIFY | STRETCH | Matlab-like. Very large. May need 060. |
| python 2.x | PSF | 2 | VERIFY | STRETCH | Python 2.7 might fit in 16MB. Very slow on 030. |
| perl 5 | Larry Wall | 2 | VERIFY | MEDIUM | Perl 5.x. Needs 8MB+. |
| gcc (native) | GNU | 2 | EXISTS | — | GG gcc exists. Very slow on 030. |
| make (GNU) | GNU | 2 | EXISTS | — | GG make exists. |
| git | Junio Hamano | 2+4 | VERIFY | STRETCH | Huge. Needs many deps. Probably 060 only. |
| vim | Bram Moolenaar | 3 | VERIFY | MEDIUM | Full vim (vs nvi). Larger but more features. |
| emacs | GNU | 3 | VERIFY | STRETCH | GNU Emacs. Very large. Needs 16MB+. |
| sqlite | D. Richard Hipp | 2 | VERIFY | MEDIUM | Embedded database. ~500KB. Good fit for 030+. |
| mutt | Michael Strstrm | 3+4 | VERIFY | MEDIUM | Email client. Needs networking. |
| irssi | Timo Sirainen | 3+4 | VERIFY | MEDIUM | IRC client. ncurses-based. |

---

# Tier 4: A4000/040 or A1200+040

**68040 @ 25MHz, 2MB Chip + 4-16MB Fast, integrated FPU**

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| All Tier 1-3 | — | — | — | — | Everything above runs well here. |
| ruby 1.8 | Matz | 2 | VERIFY | STRETCH | Old Ruby might fit. Very slow. |
| ghostscript | Artifex | 2 | VERIFY | STRETCH | PostScript interpreter. Large. |
| imagemagick | ImageMagick Studio | 2 | VERIFY | STRETCH | Image manipulation. Huge. Memory-hungry. |
| sox | Chris Bagwell | 2 | VERIFY | MEDIUM | Sound processing. Needs FPU for some ops. |

---

# Tier 5: 68060 Systems

**68060 @ 50-75MHz, 32-128MB RAM**

Fastest classic Amiga. Can run surprisingly large software.

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| All Tier 1-4 | — | — | — | — | Everything above runs well. |
| python 3.x | PSF | 2 | VERIFY | STRETCH | Might barely work with 64MB+. Very slow. |
| node.js | OpenJS | 2 | INFEASIBLE | — | Needs JIT, modern OS features. |
| rust tools | Various | 2 | INFEASIBLE | — | Rust runtime too heavy for 68k. |
| go tools | Google | 2 | INFEASIBLE | — | Go runtime not available for 68k. |

---

# Tier 6: A500/A600 (68000)

**68000 @ 7MHz, 512KB-2MB Chip RAM, OCS/ECS**

Very limited. Only the smallest, simplest tools. Our binaries already target 68000 instructions via `-m68000` for libraries, but port binaries use 68020.

| Program | Source | Cat | Status | Priority | Notes |
|---------|--------|-----|--------|----------|-------|
| Most Tier 1 CLI | OpenBSD | 1 | CANDIDATE | MEDIUM | Recompile with -m68000. Need small stack/buffers. |
| wc | OpenBSD | 1 | CANDIDATE | HIGH | Very small. Good 68000 candidate. |
| head/tail | OpenBSD | 1 | CANDIDATE | HIGH | Small enough for 512KB. |
| yes | OpenBSD | 1 | CANDIDATE | HIGH | Trivial. |
| cal | OpenBSD | 1 | CANDIDATE | MEDIUM | Should fit. |
| grep | OpenBSD | 2 | STRETCH | MEDIUM | Regex engine uses memory. Tight on 512KB. |
| less | GNU | 3 | STRETCH | LOW | Console UI on OCS is limited. |

**Note:** A500/A600 ports require recompilation with `-m68000` (our current ports use `-m68020`). The shim libraries are already built with `-m68000`. Stack sizes may need reduction. Test with `vamos -s 32` to simulate tight stack.

---

## Aminet Cross-Reference Status

### Verified (Researcher Batch 1-2, 2026-03-24)

**Text Processing — adtools/coreutils covers most:**
tr, fold, rev, expand, unexpand, paste, join, comm, fmt, nl, od, split all available via adtools/coreutils (GitHub) and/or Geek Gadgets (ixemul-dependent). jffabre.free.fr has lightweight standalone ports of some (fold, rev).

**Gaps found — no existing ports:**
- `column` — No port anywhere
- `hexdump` — BSD utility, not in GNU coreutils
- `csplit` — Unconfirmed
- `strings` — May be in GG binutils but no standalone binary

**File/Directory — mostly unported:**
- `find`, `xargs`, `cmp`, `tree`, `touch`, `file`, `basename`, `dirname` — No ports found
- `du` — AmigaOS-specific tools exist (DiskUsage, Mnemosyne) but not Unix du(1) compatible

### Pending Verification

Researchers still running for: editors/interactive, network/crypto, misc/languages.

### Our Value Proposition vs Existing Ports

Even where ports exist (GG, adtools), our ports offer:
- **No ixemul dependency** — standalone -noixemul binaries
- **Modern versions** — OpenBSD current, not 1990s GNU
- **Tested on real AmigaOS** — FS-UAE verified on every port
- **Small binaries** — OpenBSD code is minimal
- **Proper Amiga conventions** — exit codes, Ctrl-C, T: for temp, $VER strings

---

## How to Use This Catalog

1. **Pick a CANDIDATE item** with HIGH priority from the current tier
2. Run `aminet-researcher` agent to verify status if still VERIFY
3. Use `/port-project` to port it
4. Update status to PORTED and add to PORTS.md
5. Work through HIGH → MEDIUM → LOW priority

## Recommended Porting Order (Tier 1)

Based on research, highest-value candidates for stock A1200:

1. **strings** — No port exists. Invaluable for Amiga developers.
2. **find** — No port exists. Critical for shell scripting.
3. **xargs** — No port exists. Pairs with find.
4. **basename/dirname** — No ports. Trivial. Enable shell scripts.
5. **cmp** — No port. Complements our diff.
6. **tree** — No port. Very popular utility.
7. **awk** — GG gawk exists but bloated. OpenBSD awk is tiny.
8. **mg** — Micro Emacs editor. Would be first real editor port.
9. **nano** — Very popular request from community.
10. **touch** — No port. Build systems need it.
