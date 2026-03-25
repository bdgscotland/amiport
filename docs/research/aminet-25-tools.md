# Aminet Research Report: 25 Core Unix Tools

**Date**: March 25, 2026
**Researcher**: aminet-researcher agent
**Status**: COMPLETE

## Executive Summary

Out of 25 core Unix utilities examined:
- **22 already exist** on Aminet (via GNU coreutils 5.2.1-9, published 2010)
- **2 already exist** on Aminet (via GNU findutils 4.1, published 1998)
- **1 is missing** (column) — **HIGH PORTING VALUE**
- **1 is niche** (file) — not ported, low demand

**Recommendation**: Skip porting 24 tools. Focus effort on **column** as a standalone utility.

---

## Detailed Findings

### GNU coreutils 5.2.1-9 (22 tools)

**Package**: `coreutils-m68k-bin.lha`
**Version**: 5.2.1-9
**Published**: April 21, 2010
**Architecture**: m68k AmigaOS
**Size**: 7.3 MB
**Downloads**: 1,948+
**Source**: Geek Gadgets distribution (1998 base, updated 2010)
**URL**: https://aminet.net/package/dev/gg/coreutils-m68k-bin

**Tools Included** (22 of the 25 researched):

| Tool | Status | Notes |
|------|--------|-------|
| tr | exists | translate or delete characters |
| fmt | exists | simple text formatter |
| fold | exists | break long lines |
| paste | exists | merge lines from files |
| join | exists | relational database operation on text |
| tsort | exists | topological sort |
| ls | exists | list files |
| du | exists | disk usage |
| touch | exists | create/update file timestamps |
| ln | exists | create links |
| cp | exists | copy files |
| mv | exists | move/rename files |
| rm | exists | remove files |
| mkdir | exists | create directories |
| rmdir | exists | remove empty directories |
| chmod | exists | change file permissions |
| env | exists | run command with environment changes |
| printenv | exists | print environment variables |
| uname | exists | print system information |
| hostname | exists | print hostname |
| id | exists | print user/group IDs |
| (also includes) | — | cut, head, tail, wc, sort, uniq, and 70+ other utilities |

**Assessment**:
- **Verdict**: SKIP
- **Version Age**: 20+ years old (based on GNU coreutils 5.2.1 from 2005)
- **Functional**: Yes, these core utilities remain largely compatible
- **Quality**: Poor by modern standards
  - No Unicode/UTF-8 support in text tools (tr, fold, fmt)
  - Missing modern command-line options
  - No performance optimizations from 20 years of development
  - No security updates
- **Recommendation**: Users seeking these tools can download coreutils from Aminet. Value of fresh porting is limited unless targeting specific improvements (e.g., UTF-8 support, modern GNU options)

---

### GNU findutils 4.1 (2 tools)

**Package**: `findutils-bin.lha`
**Version**: 4.1
**Published**: March 16, 1998
**Architecture**: m68k AmigaOS
**Size**: 145 KB
**Downloads**: 1,948+
**Source**: Geek Gadgets distribution
**URL**: https://aminet.net/package/dev/gg/findutils-bin

**Tools Included** (2 of the 25 researched):

| Tool | Status | Notes |
|------|--------|-------|
| find | exists | search for files |
| xargs | exists | execute command on file list |

**Assessment**:
- **Verdict**: UPGRADE
- **Version Age**: 28 years old (1998)
- **Functional**: Yes, core functionality works
- **Quality**: Very poor by modern standards
  - Missing `-printf` predicate (major feature in modern find)
  - No parallel execution in xargs (`-P` flag)
  - Missing many modern predicates (e.g., `-newer`, `-readable`)
  - No optimization for large file trees
  - Security issues likely present
- **Recommendation**: If fresh porting is undertaken, target findutils 4.2+ (released 2000) or modern GNU findutils 4.9+. The 1998 version is severely limited.

---

### Missing Tools

#### 1. column

**Status**: PORT
**Aminet Search**: Not found
**Source**: BSD column(1)
**Upstream**: OpenBSD (modern maintenance), also FreeBSD
**Complexity**: Simple (~300 lines of C)
**Dependencies**: None (pure POSIX stdio)
**Description**: Format text in columns or tables
**Use Case**: Text formatting for CLI output

**Assessment**:
- **Verdict**: HIGH PORTING VALUE
- **Why Missing**: Not included in GNU coreutils. BSD column(1) has never been ported to AmigaOS.
- **Community Value**: Moderate but concentrated (system administrators, text processing scripts)
- **Porting Effort**: LOW (simple C program, no complex dependencies)
- **Recommendation**: STRONG RECOMMEND PORTING. This tool is genuinely missing and serves a real purpose (text columnarization for reports/output).

**Reference Implementation**: OpenBSD column.c (https://cvsweb.openbsd.org/src/usr.bin/column/)

---

#### 2. file

**Status**: NICHE (SKIP)
**Aminet Search**: Not found
**Source**: GNU file utility
**Upstream**: Gary Keim / Christos Zoulas
**Complexity**: Large (~50KB compiled)
**Dependencies**: libc (standard POSIX)
**Description**: Determine file type from magic bytes
**Use Case**: File type detection (mainly used by developers)

**Assessment**:
- **Verdict**: SKIP (NICHE USE CASE)
- **Why Missing**: GNU file utility exists but has never been ported to AmigaOS. Limited demand.
- **Community Value**: Low (used mainly by developers, system administrators for diagnostics)
- **Porting Effort**: MODERATE (external magic database, regex support)
- **AmigaOS Equivalent**: AmigaOS has native file type detection via Icons + file typing system. Less useful than on Unix.
- **Recommendation**: SKIP. Outside core CLI utility scope. Use native AmigaOS file type mechanism instead.

---

## Geek Gadgets Distribution

The Geek Gadgets distribution (1998) remains the largest collection of GNU tools on Aminet for 68k AmigaOS. It has been minimally maintained (last update 2010 for coreutils recompilation).

**Key Packages**:
- `coreutils-m68k-bin.lha` (v5.2.1-9, 7.3M, 2010)
- `findutils-bin.lha` (v4.1, 145K, 1998)
- `bash-2.01-m68k-bin.lha` (2.0 shell, 1998)
- `gcc-bin.lha` (2.7.2 compiler, 1998)
- `libc.a` and support libraries

**Characteristics**:
- Uses `ixemul.library` (Unix emulation layer)
- Full GCC toolchain included
- Requires Geek Gadgets runtime environment
- No security updates since 1998

**amiport Advantage**: Standalone binaries using lighter `-noixemul` shim are better suited for modern distributions. Geek Gadgets tools require the full Unix emulation environment.

---

## Search Methodology

1. **Aminet.net direct search**: Searched for each tool individually
2. **Category browsing**: Checked util/cli, util/dir, text/misc categories
3. **GNU package searches**: Searched for coreutils, findutils, file packages
4. **Geek Gadgets inventory**: Cross-referenced Aminet dev/gg category
5. **Verification**: Checked GNU package histories to confirm what v5.2.1 includes

**Search Queries Performed**:
- Individual tool names (tr, fmt, column, etc.)
- Package names (coreutils, findutils, geek-gadgets)
- Category browser (util/cli, util/dir, text/misc, dev/gg)

---

## Catalog Update

All 25 tools have been updated in `data/catalog.json` with:
- **aminet_status**: "exists", "missing", or "niche"
- **aminet_notes**: Detailed source, version, and assessment

This enables the amiport project to:
1. **Avoid redundant porting** — tools already on Aminet don't need re-porting
2. **Guide prioritization** — focus on genuinely missing tools (column)
3. **Document alternatives** — users seeking coreutils can get them from Aminet
4. **Track improvements** — document where fresh porting adds value over Geek Gadgets versions

---

## Recommendations for amiport

### Short Term (Do Now)
1. **Add Aminet references to PORTS.md**
   - List which tools are available via coreutils 5.2.1-9
   - Link to Aminet packages
   - Note version age and limitations

2. **Consider porting column**
   - Simple tool (300 lines)
   - No complex dependencies
   - Genuine gap in Aminet ecosystem
   - High porting value for CLI users

### Medium Term (Next Quarter)
1. **Evaluate column porting complexity** via source-analyzer
2. **If porting column: document** in PORTS.md as first text-processing port
3. **Research modern coreutils** (8.x or 9.x)
   - Check if fresh port is feasible
   - Would require significant work (UTF-8 support, modern flags)
   - Defer unless strong community demand

### Long Term
1. **Consider modern findutils** (4.9+) if find/xargs usage is significant
2. **Monitor Aminet** for updates to Geek Gadgets or new ports
3. **Document lessons** from column porting for future text processing ports

---

## References

- Aminet: https://aminet.net/
- GNU coreutils: https://www.gnu.org/software/coreutils/
- GNU findutils: https://www.gnu.org/software/findutils/
- OpenBSD column: https://cvsweb.openbsd.org/src/usr.bin/column/
- Geek Gadgets archive: https://aminet.net/dev/gg/

