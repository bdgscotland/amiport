# amiport — TODO

## Shims & Infrastructure

- [ ] **Amiga wildcard/glob expansion shim** — `amiport_glob_args()` to expand `#?` and `*` patterns in argv at program startup (Amiga shells don't glob-expand like Unix). Support both Amiga (`#?`, `~`, `(a|b)`) and Unix (`*`, `?`, `[a-z]`) patterns. Call from boilerplate startup.
- [ ] **Amiga path translation shim** — `amiport_path_translate()` for `/tmp` → `T:`, `/dev/null` → `NIL:`, `~/` → `HOME:`, and other common path patterns that break ports.
- [ ] **ReadArgs compatibility layer** — Dual-mode argument parser so ports accept both Unix getopt and native AmigaOS ReadArgs templates (`PATTERN/A,ALL/S,QUIET/S`). Makes ports feel native.
- [ ] **Command naming collision reference** — Doc listing native Amiga commands that collide with Unix tool names (`date`, `join`, `sort`, `type`) with recommended conventions (prefix with `g`, use `C:` assigns, etc.)

## Skills & Agents

- [ ] **`/validate-port` skill** — Automated port validation: run native version on host, run Amiga version via vamos, diff output, generate compatibility report.
- [ ] **Port target catalog** — Curated doc of high-value ports with complexity estimates (see list below).

## Toolchain & Docs

- [ ] **Document bebbo's vamos fork** — `pip install git+https://github.com/bebbo/amitools.git` gives improved vamos with better compat than upstream cnvogelg/amitools.
- [ ] **Test with GCC 13.3/15.2 branches** — bebbo's toolchain now has `amiga13.3` and `amiga15.2` branches with register parameter support. Evaluate whether we can offer these as optional target profiles.
- [ ] **Verify newlib-availability.md** — bebbo has been cross-porting functions between libnix and newlib; our reference may already be stale for some entries.

## High-Value Port Targets

Priority order based on community demand and Aminet staleness:

| Tool | Aminet Status | Est. Complexity | Regex Needed | Notes |
|------|--------------|-----------------|--------------|-------|
| **grep** | v1.6 (1993) | Moderate | Yes | Most-wanted. Uses our new regex emu. |
| **sed** | ancient | Moderate | Yes | Second most-wanted. Regex emu. |
| **sort** | v1.0 (1993) | Easy | No | Pure computation, locale stubs. |
| **cut** | unknown/missing | Trivial | No | Almost no POSIX deps. |
| **tr** | unknown/missing | Trivial | No | Pure character translation. |
| **uniq** | unknown/missing | Trivial | No | Line comparison only. |
| **tail** | unknown/missing | Easy | No | File I/O + seek. |
| **tee** | unknown/missing | Trivial | No | Reads stdin, writes to file + stdout. |
| **xargs** | unknown/missing | Easy | No | Arg passing, needs glob shim. |
| **less/more** | ancient | Moderate | No | Terminal control (raw console). |
| **basename/dirname** | unknown | Trivial | No | Pure string manipulation. |
| **env** | unknown | Trivial | No | Uses GetVar/SetVar. |
| **true/false** | trivial | Trivial | No | Literally exit 0 / exit 1. |

## Research Inspiration

Sources that informed this list:
- Aminet util/cli — only 2 uploads in 2 years, most tools from 1993
- jffabre's Unix commands page — documents gaps and wildcard issues
- English Amiga Board porting threads — wildcard/path issues are #1 complaint
- bebbo/amiga-gcc Codeberg — GCC 13.3+ now available
- AmigaOS 3.3 nearing release — confirms 3.x target is correct
