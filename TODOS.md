# TODOS

## Shim Library Enhancements

### ~~Add `amiport_dup()` and `amiport_dup2()`~~ — DONE

Implemented with parallel `fd_closeable[]` array and scan-on-close for shared BPTRs. 258-line test suite in `tests/shim/test_dup.c`. Moved from "Planned" to Tier 1 in `docs/posix-tiers.md`.

---

### ~~Add `amiport_pipe()` for basic pipe emulation~~ — DONE

Implemented as Tier 2 emulation (`amiport_emu_pipe()`) using AmigaDOS PIPE: device with unique channel names via task address + counter. Test suite in `tests/emu/test_pipe.c`. Documented caveats: named (not anonymous) pipes, different blocking/buffering, no SIGPIPE, less reliable EOF detection.

---

### Amiga path translation shim

**What:** `amiport_path_translate()` for `/tmp` → `T:`, `/dev/null` → `NIL:`, `~/` → `HOME:`.

**Why:** Common Unix path patterns break ports. An automatic translation layer at fopen/open time would fix this transparently. Could be hooked into `amiport_open()` and `amiport_fopen()` so every port benefits without source changes.

**Priority:** High — silently fixes a whole class of bugs across all ports.

---

### ~~Amiga wildcard/glob expansion shim~~ — DONE

Implemented both POSIX `glob()`/`globfree()` (Tier 1) and automatic argv wildcard expansion via `amiport_expand_argv()`. Supports both Unix (`*`, `?`, `[...]`) and AmigaOS (`#?`, `(a|b)`) patterns via transparent Amiga-to-Unix translation. Opt-out via `int __nowild = 1;` matching SAS/C convention. 32 tests across two test suites (test_glob: 22, test_argv_expand: 10).

---

### Re-port grep and sed with __nowild support

**What:** Re-run grep and sed through `/port-project` so the code-transformer emits `int __nowild = 1;` automatically.

**Why:** With argv expansion now in libamiport, these ports will incorrectly expand pattern arguments unless suppressed. E.g., `grep *.c file.txt` would expand `*.c` before grep sees it as a regex. Rather than manually patching, re-porting lets the pipeline handle it correctly.

**Priority:** High — required before next libamiport release.

**Depends on:** Glob/argv shim being complete (done).

---

## FS-UAE Testing

### ~~FS-UAE serial console debug mode~~ — SUPERSEDED

Superseded by the autonomous debug agent design (ADR-016). The serial→TCP mechanism is now used for automated Enforcer crash capture via `test-fsemu.sh --debug`, which is strictly more capable than the interactive shell approach originally proposed here.

---

### Verify bgdbserver TCP routing through FS-UAE

**What:** Run bgdbserver inside FS-UAE listening on a TCP port, verify `telnet localhost <port>` connects from the host.

**Why:** This is the blocking verification gate for Layer 2 (GDB debugging) of the autonomous debug agent. Research confirmed FS-UAE bsdsocket uses real host sockets, so this *should* work — but needs empirical verification with bgdbserver specifically.

**Details:** Test with `bsdsocket_library = 1` in FS-UAE config. If it fails, the alternative is `astub` (a serial-based GDB stub), not "bgdbserver over serial" — bgdbserver is TCP-only. Requires ~15 min of manual testing.

**Depends on:** Debug tools downloaded via `make setup-debug-tools`.

---

### Build m68k-amigaos-gdb in Docker toolchain image

**What:** Add `m68k-amigaos-gdb` to the bebbo-gcc Docker image build. Currently not built by default — requires a separate build step with a newer bison version than macOS ships.

**Why:** Layer 2 (GDB debugging) needs `m68k-amigaos-gdb` on the host to connect to bgdbserver. Without it, even if bgdbserver TCP works, GDB sessions aren't possible.

**Details:** May require adding bison to the Docker image's build dependencies. Also useful for developers wanting to debug Amiga code manually outside the agent pipeline.

**Depends on:** bgdbserver TCP verification (above TODO).

---

### CrashLogger: Capture Guru Meditation error codes via SetFunction()

**What:** Write a small AmigaOS program (`CrashLogger`) that patches `exec.library/Alert()` using `SetFunction()` to intercept Guru Meditations before the alert requester appears. The patched Alert writes the alert number, PC, and task name to `WORK:crash-data.txt`, then calls the original Alert.

**Why:** When a ported program crashes on FS-UAE, the Guru Meditation error code (e.g., `#8000000B` = ACPU_LineF) is only visible in the emulator window. The test harness sees only a timeout — Claude never gets the error code. This is the single biggest gap in crash diagnosis: the information exists but is trapped inside the emulator display.

**How:** `SetFunction(SysBase, -108, newAlertFunc)` patches Alert (LVO -108). The new function opens `WORK:crash-data.txt`, writes the alertNum (D7), then JMPs to the original. Load CrashLogger in User-Startup before the test runner. The test-fsemu.sh script reads crash-data.txt on timeout and prints the error code in the diagnosis.

**Research:** SetFunction is explicitly supported for this use case (ADCD autodocs). Alert's LVO is -108 in exec.library. The approach is safe (task-local write, then original Alert runs normally). Enforcer and MungWall use the same technique.

**Priority:** P1 (blocks autonomous crash diagnosis in non-debug mode)

**Effort:** S (human: ~2 hours / CC: ~30 min)

---

### Runtime Profiler: ReadEClock-based function timing for ported programs

**What:** Add `<amiport/profile.h>` with `AMIPORT_PROFILE_BEGIN(name)` / `AMIPORT_PROFILE_END(name)` macros that use `timer.device/ReadEClock()` to measure function execution time at sub-microsecond resolution, and print a sorted summary at exit.

**Why:** The perf-optimizer agent does static analysis only — it can't measure actual runtime. On 7MHz 68000, the difference between fgetc-per-char and fgets-per-line is 3-5x, but we can only estimate this. A real profiler would let us verify optimizations and find bottlenecks we miss.

**Design (informed by ADCD timer.device docs):**
```c
#include <amiport/profile.h>

/* In ported source — zero overhead when AMIPORT_PROFILE is not defined */
AMIPORT_PROFILE_BEGIN("readhash");
/* ... function body ... */
AMIPORT_PROFILE_END("readhash");
```

Implementation:
1. Open `timer.device` UNIT_ECLOCK once at startup via `amiport_profile_init()`
2. `ReadEClock()` returns a 64-bit tick count + E-Clock frequency (tics/sec)
3. Each BEGIN/END pair computes `(time2.ev_lo - time1.ev_lo)` and accumulates into a static table
4. `amiport_profile_summary()` (registered via atexit) prints: function name, call count, total ticks, avg ticks, % of total
5. Sort by total ticks descending — hottest function first
6. When `AMIPORT_PROFILE` is not defined, macros expand to nothing (zero overhead)

Key ADCD references:
- `autodocs-3.5/timer-device-readeclock-2.md` — ReadEClock API ("low overhead function designed so that very short intervals may be timed")
- `devices/13-timer-device-e-clock-time-and-its-relationship-to-actual.md` — EClockVal subtraction pattern
- `autodocs-3.5/timer-device-background-2.md` — UNIT_ECLOCK details, 2μs precision
- `examples/devices/13-timer-device-e-clock-time-and-its-relationship-to-actual.c` — canonical usage example

E-Clock frequency: ~709,379 Hz (PAL) or ~715,909 Hz (NTSC). Each tick ≈ 1.4μs. ReadEClock itself costs ~10μs per call, so profiling overhead is ~20μs per instrumented function call — acceptable for functions called <100K times, but will distort tight inner loops. For inner loops, profile the outer function instead.

**Output format:**
```
=== amiport profiler summary ===
Function         Calls    Total(ms)  Avg(us)  %
readhash         2000     1234.5     617.2    42.3%
check            1000     890.1      890.1    30.5%
skipline         200      234.5      1172.5   8.0%
...
Total measured: 2918.3ms
```

**Scope:** New header `lib/posix-shim/include/amiport/profile.h`, new source `lib/posix-shim/src/profile.c`. Link with `-lamiport` (already in all port Makefiles). Build with `make build-shim`. Enable per-port via `CFLAGS += -DAMIPORT_PROFILE` in the port's Makefile.

**Priority:** P2

**Effort:** M (human: ~4 hours / CC: ~1 hour)

---

### FS-UAE Integration Testing in CI

**What:** Run `make test-fsemu` in GitHub Actions for every push.

**Why:** Currently only vamos tests run in CI. FS-UAE tests catch bugs that vamos misses (console I/O, real AmigaOS behavior).

**Details:** Requires FS-UAE + Kickstart 3.1 ROM in CI runner (licensing issue), headless mode. Consider running only on main branch.

**Depends on:** All FS-UAE test infrastructure shipping first.

**Priority:** P2

**Effort:** L (human: ~1 week / CC: ~1 hour)

---

## Infrastructure

### ~~Add CI/CD with GitHub Actions~~ — DONE

GitHub Actions workflow deployed and running on every push to main. Builds all four shim libraries (posix-shim, posix-emu, console-shim, bsdsocket-shim), runs all tests, validates doc consistency and agent frontmatter, builds and tests all example ports AND all production ports via `make test-ports`. Production port tests gate CI (no continue-on-error).

---

### Agent Cost/Token Tracking

**What:** Track which agents consume the most time during `/port-project` runs.

**Why:** With 10 agents potentially dispatching during a single port, there's no visibility into cost distribution. Knowing which stages are expensive informs model selection decisions.

**Details:** Claude Code doesn't expose token counts to scripts, so track wall-clock time per agent dispatch instead. Log agent name, model, and duration to PORT.md. Could use timestamps around Agent tool calls in the port-project skill.

**Depends on:** Nothing — can be implemented independently.

---

### Verify newlib-availability.md

**What:** Audit `docs/references/newlib-availability.md` against current bebbo-gcc libnix.

**Why:** bebbo has been cross-porting functions between libnix and newlib; our reference may be stale.

---

### ~~68k Hardware Reference for Crash Debugging~~ — DONE

Created `docs/references/68k-hardware.md` with 8 sections: crash signature quick reference (action-first), Amiga memory map, 68000 vs 68020 addressing (24-bit wrapping), register conventions, stack behavior, trap frames & Guru codes, vamos vs real hardware differences, and instruction reference (stub for Layer 2 GDB). Hardware address patterns merged into debug-agent's Crash Classification section. Linked from CLAUDE.md Key References.

---

### ~~Amiga Hardware Expert Agent~~ — DONE

Created `hardware-expert` agent (12th agent) with 6 research-backed knowledge domains: CPU variant matrix (EC020/EC030 distinctions), chipset generations (OCS/ECS/AGA), address space architecture, Kickstart ROM & LVO calling convention, DMA & bus arbitration, and common hardware misconceptions. Dual-role: on-demand consultant (debug-agent, perf-optimizer, port-coordinator can escalate) + proactive auditor for reference docs. Added Agent tool to debug-agent for escalation path. Fixed perf-optimizer CPU table to correctly distinguish 68EC020. Motivated by the EC020/68020 mistake in 68k-hardware.md that the user caught.

---

### libnix Limitations Reference

**What:** Create `docs/references/libnix-limitations.md` documenting known gaps and quirks in bebbo-gcc's libnix C runtime.

**Why:** The vsnprintf(NULL, 0, ...) crash happened because the code-transformer generated C99 code that libnix doesn't support. Without a limitations reference, each port independently discovers the same pitfalls.

**Details:** Known issues to document: vsnprintf NULL destination, missing vasprintf/asprintf, locale stubs, signal behavior, thread safety (none), errno behavior, missing POSIX functions. Cross-reference with `newlib-availability.md`. Add to code-transformer and build-manager agent instructions as mandatory reading.

**Depends on:** Audit of current `docs/references/newlib-availability.md` for staleness.

---

### diff Port vamos Crash Investigation

**What:** Debug the diff port's vamos crash (InvalidMemoryAccessError at PC=0x004c1e, stack overflow/addressing issue).

**Why:** diff crashes on vamos when processing files — pre-existing issue, not caused by recent changes. The crash is in the err()/strerror() call path. Stack is allocated at high addresses that vamos can't access (likely 24-bit address wrapping or vamos memory validator bug).

**Details:** The binary works fine for `--help` output but crashes when processing actual files. Binary from main branch also crashes. Need to investigate whether this is a vamos limitation (Category 3+ port that needs FS-UAE) or a fixable memory layout issue. Try compiling with `-mcpu=68020` or adjusting linker script to place stack in lower memory.

---

## Reference Documentation Improvements

### TOPICS.md — Semantic task-to-ADCD page mapping

**What:** Curated mapping of ~30-50 common porting tasks (memory allocation, file I/O, window management, message ports, device I/O, library management, input handling, timer operations, etc.) to relevant ADCD pages.

**Why:** Gives agents "semantic search" for free — no database needed. Currently an agent wanting to know "how do I allocate memory on AmigaOS?" has to know the right filename. A topics index lets them look up the *task* and find all relevant pages.

**Details:** Hand-curated, not auto-generated. Each topic maps to 2-5 ADCD pages with a one-line rationale for why each page is relevant. Lives at `docs/references/adcd/TOPICS.md`.

---

### LIBRARIES.md — Reverse lookup from library name to ADCD pages

**What:** Reverse index from library/device name (e.g., `timer.device`, `exec.library`, `dos.library`) to all ADCD pages that document it.

**Why:** Currently agents have to parse `manifest.json` to find "all timer.device docs." A flat lookup table is faster and grep-friendly.

**Details:** Auto-generatable from `manifest.json` paths in one pass — low effort. One table per library/device with page title and path. Lives at `docs/references/adcd/LIBRARIES.md`.

---

### Manifest summaries — One-line descriptions in manifest.json (P3)

**What:** Add a `description` field to each entry in `docs/references/adcd/manifest.json` with a one-line summary of what the page covers.

**Why:** Agents can grep manifest.json for topics without reading every file. Currently the manifest has paths and titles but no content summary, forcing agents to open files speculatively.

**Details:** ~6,700 entries to process — large batch job. TOPICS.md (curated, ~50 entries) likely provides 80% of the value at 1% of the effort. Consider deferring until TOPICS.md proves insufficient.

---

### Agent prompt-level ADCD consultation (P2)

**What:** Update the `code-transformer` agent to consult `INCLUDES.json` for targeted ADCD chapter reads during transformation decisions.

**Why:** The code-transformer currently applies mechanical transformation rules. With ADCD access, it could make better decisions about *how* to use AmigaOS APIs — not just which function to call, but which flags, error handling patterns, and idioms are correct.

**Details:** Low priority (P2) — the current transformation rules work well for most cases. This would improve quality for complex transformations involving device I/O, message ports, or Intuition. Depends on TOPICS.md and LIBRARIES.md existing first.

---

## Website & Package Manager

### amiport.platesteel.net — Website + Package Index + amiget CLI

**What:** Three-part system: (1) project website on Dreamhost shared hosting with PHP API, (2) browsable package index with download tracking, (3) `amiget` CLI for classic AmigaOS 3.x that fetches and installs packages over HTTP.

**Why:** No package manager exists for classic 68k AmigaOS. Aminet is a file dump. Grunch serves OS4/MorphOS only. This fills the gap and gives amiport a web home.

**Design doc:** `~/.gstack/projects/bdgscotland-amiport/duncan-main-design-20260322-113337.md` (APPROVED)
**Design system:** `DESIGN.md` (Amiga MUI on warm gray — Commodore palette, no blue)
**CEO plan:** `~/.gstack/projects/bdgscotland-amiport/ceo-plans/2026-03-22-site-redesign.md` (6 scope expansions accepted)

**Implementation phases:**
1. [ ] Configure `amiport.platesteel.net` subdomain in Dreamhost panel
2. [ ] Build website skeleton (landing page, package browser, API endpoints) — `site/`
3. [ ] Create per-port `package.json` files for all 11 ports
4. [ ] Create `/deploy-site` skill + `site-manager` agent
5. [ ] Build `lib/http-shim/` (HTTP/1.0 client on top of bsdsocket-shim)
6. [ ] Port `amiget` CLI through `/port-project` pipeline — `ports/amiget/`
7. [ ] Create `/publish-package` skill (integrates with port completion)
8. [ ] Announce on Amiga forums

**Key decisions:**
- PHP + static frontend (no database, flat-file JSON store)
- Query parameters for API (`/api/v1/download.php?name=grep`), not pretty URLs
- HTTP only for Amiga CLI (no TLS), HTTPS for browsers
- Per-port package.json files committed to repo (no markdown parsing)
- `lib/http-shim/` as reusable library from day one
- Unit test HTTP/JSON parsing in vamos; network integration tests via FS-UAE

---

## Port Targets

### Completed Ports

| Port | Category | Version | Aminet |
|------|----------|---------|--------|
| cal | 1 (CLI) | 1.32 | Submitted |
| cut | 1 (CLI) | 1.28 | Submitted |
| diff | 1 (CLI) | 1.95 | Packaged |
| grep | 1 (CLI) | 1.68 | Packaged |
| sed | 1 (CLI) | 1.47 | Packaged |
| lua | 2 (Scripting) | 5.4.7 | Packaged |

### Category 1 (CLI) — Next Candidates

| Tool | Aminet Status | Complexity | Notes |
|------|--------------|------------|-------|
| tr | missing | Trivial | Character translation |
| uniq | missing | Trivial | Line comparison |
| tee | missing | Trivial | stdin → file + stdout |
| basename/dirname | missing | Trivial | String manipulation |
| tail | missing | Easy | File I/O + seek |
| sort | v1.0 (1993) | Easy | Pure computation, locale stubs — outdated version on Aminet |
| xargs | missing | Easy | Benefits from glob shim |

### Category 3 (Console UI) — less

Console-shim library complete (ADR-009). `less` is the first target. Needs window size detection via Intuition.

### ~~Category 4 (Network) — wget-lite~~ — SUBSUMED

Subsumed by the `amiget` port (amiport package manager CLI). `amiget` is the first Category 4 consumer of bsdsocket-shim — it implements a full HTTP/1.0 client via `lib/http-shim/` for downloading packages. The http-shim component serves as the reusable HTTP layer that wget-lite would have created.
