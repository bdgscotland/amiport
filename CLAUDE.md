# amiport — Claude Code Project Instructions

## What This Project Is

amiport is an AI-powered toolkit for porting POSIX/Linux C software to the Commodore Amiga. Claude is the primary porting agent — this project provides the skills, agents, reference docs, and shim libraries that enable automated porting.

## Architecture

The porting pipeline has 5 stages, each backed by a Claude skill:

1. **Analyze** (`/analyze-source`) — Scan source for portability issues
2. **Transform** (`/transform-source`) — Replace POSIX calls with Amiga equivalents
3. **Build** (`/build-amiga`) — Cross-compile with bebbo-gcc or VBCC
4. **Test** (`/test-amiga`) — Run in vamos emulator, verify output
5. **Orchestrate** (`/port-project`) — Run the full pipeline end-to-end

## Codebase Map

- `.claude/skills/` — Skill definitions for each pipeline stage
- `.claude/agents/` — Agent definitions (source-analyzer, code-transformer, build-manager, test-runner, port-coordinator, dependency-auditor)
- `lib/posix-shim/` — Tier 1: Direct POSIX-to-AmigaOS wrappers (`amiport_*` functions)
- `lib/posix-emu/` — Tier 2: Approximate POSIX emulation with documented caveats (`amiport_emu_*` functions)
- `lib/console-shim/` — Minimal ncurses API mapped to Amiga console.device ANSI escapes (ADR-009)
- `lib/bsdsocket-shim/` — BSD socket API via bsdsocket.library with auto lifecycle (ADR-010)
- `toolchain/` — Cross-compiler Docker images, build scripts, target profiles
- `toolchain/uaequit/` — UAEQuit helper binary (signals FS-UAE to exit after test runs)
- `toolchain/templates/test-runner.rexx` — ARexx test harness template for FS-UAE automated testing
- `docs/` — Architecture docs, API mapping tables, porting guide, tier classification
- `docs/references/adcd/` — Complete ADCD 2.1 in agent-optimized markdown (Libraries, Devices, Hardware, Amiga Mail, Autodocs)
- `tests/shim/` — Tier 1 posix-shim unit tests (11 test files)
- `tests/emu/` — Tier 2 posix-emu tests (4 test files)
- `tests/console/` — Console shim tests (non-interactive)
- `tests/net/` — BSD socket shim tests (inet, fd tracking)
- `tests/common/` — Shared test framework
- `ports/` — Output directory for real ports (each port gets original/, ported/, Makefile, PORT.md)
- `ports/templates/` — Canonical templates for per-port artifacts (Makefile, PORT.md, .readme, directory structure)
- `examples/` — Reference ports for testing the pipeline (wc, head, mini-find)

## Coding Conventions for Amiga C

- Use **ANSI C89** for maximum compatibility with classic AmigaOS (no C99 unless targeting OS 4.x)
- Include `<proto/*.h>` headers for Amiga system calls (not `<clib/*.h>` pragmas)
- Use Amiga types (`LONG`, `ULONG`, `STRPTR`, `BPTR`, `APTR`) when interfacing with OS libraries
- Use `amiport_*` shim wrappers from `lib/posix-shim/` for Tier 1 (direct mapping) rather than raw AmigaOS calls
- Use `amiport_emu_*` wrappers from `lib/posix-emu/` for Tier 2 (emulation) with caveat comments
- Always include Amiga version string: `static const char *verstag = "$VER: progname X.Y (DD.MM.YYYY)";` — use the **upstream version** (e.g., `1.68` for OpenBSD rev 1.68, `5.4.7` for Lua), not a generic `1.0`. Use current date.
- **Use Amiga exit codes**, not POSIX: `exit(0)` is fine (RETURN_OK), but `exit(1)` is wrong — use `exit(10)` for errors (RETURN_ERROR) and `exit(20)` for fatal errors (RETURN_FAIL). Amiga scripts use `IF WARN` (>=5), `IF ERROR` (>=10), `IF FAIL` (>=20) — exit code 1 is invisible to them.

## Using the Pipeline — CRITICAL (ENFORCED BY HOOKS)

**Agent dispatch is MANDATORY. A PreToolUse hook (`enforce-agents.sh`) blocks direct edits to `ported/*.c` files. You MUST use the pipeline agents — there is no manual path.**

- **Port a project:** `/port-project <path>` — orchestrates the full pipeline with mandatory agent dispatch at each stage
- **Analyze:** dispatch `source-analyzer` agent (runs stub value impact analysis)
- **Transform:** dispatch `code-transformer` agent (hook blocks manual edits to ported/)
- **Build:** dispatch `build-manager` agent (hook blocks direct gcc calls)
- **Test:** dispatch `test-runner` agent
- **Debug crashes:** dispatch `debug-agent` or use `/debug-amiga`
- **Extend shim:** `/extend-shim <function-name>`
- **Review:** `/review-amiga` (runs automated logic bug grep checks)

The `/port-project` skill has GATE checks — it will not proceed to the next stage until the current stage's agent has returned successfully.

**Available agents:**
| Agent | When to dispatch |
|-------|-----------------|
| `aminet-researcher` | Before any port — check if it already exists |
| `source-analyzer` | Stage 1 — portability analysis |
| `code-transformer` | Stage 3 — source transformation |
| `build-manager` | Stage 4 — cross-compilation and error fixing |
| `test-runner` | Stage 5 — vamos testing |
| `port-coordinator` | Dispatched by /port-project for complex multi-file ports requiring judgment calls. Not invoked directly. |
| `dependency-auditor` | Before complex ports — audit external library dependencies |
| `debug-agent` | When a port crashes at runtime — autonomous Enforcer-based crash diagnosis and fix loop |
| `memory-checker` | **Mandatory** Stage 6b — memory leak detection, double-free, allocation safety |
| `perf-optimizer` | Optional Stage 6c — 68k instruction timing and loop optimization |
| `hardware-expert` | Hardware architecture validation — on-demand consultant + proactive doc auditor. Dispatch when agents need hardware facts (address space, CPU variants, chipset capabilities). |
| `test-designer` | Designs comprehensive FS-UAE test suites by analyzing source code, flags, exit codes, and error paths |
| `aminet-publisher` | Publishing — curated, never automatic |

**Post-port quality gates:**
- `/review-amiga <path>` — Amiga-specific code review (Stage 6a: stack safety, BPTR handling, conventions)
- `memory-checker` agent — **Mandatory** (Stage 6b). Finds memory leaks, double-frees, unsafe realloc. **AmigaOS has no memory protection or GC.** Every malloc/realloc without a matching free leaks permanently until reboot.
- `perf-optimizer` agent — Optional (Stage 6c). 68k performance tuning for performance-critical ports.

## Documentation Rules — IMPORTANT

**When adding or changing any skill, agent, pipeline stage, library, or architectural decision:**

1. **CLAUDE.md** — Update the pipeline, agent table, and any affected sections
2. **README.md** — Update the skills table, agents table, pipeline diagram, and make targets
3. **docs/architecture.md** — Update the pipeline ASCII diagram and component tables
4. **docs/porting-guide.md** — Update the step-by-step workflow
5. **.claude/skills/port-project/SKILL.md** — Update pipeline stages if affected
6. **docs/adr/** — Create a new ADR for architectural decisions; update README.md index
7. **docs/pdr/** — Create a new PDR for product/scope decisions; update README.md index

**When completing a port (`/port-project`) or publishing to Aminet:**

8. **PORTS.md** — Add the port to the catalog table (name, version, description, category, source, status). Update Aminet tracking table after publishing.
9. **README.md** — Add the port to the Ports summary table

**Do not consider a change complete until all affected documentation is updated.** A new skill without README/architecture/porting-guide references is incomplete. An ADR without an index entry is lost. A port without a PORTS.md entry is invisible.

## Porting Rules

- **Use port templates** — read `ports/templates/STRUCTURE.md` and copy `Makefile.template`, `PORT.md.template`, `readme.template` when setting up a new port. Fill in `__PLACEHOLDER__` variables.
- **All files stay inside the port directory** — never create build artifacts, test files, or temporary files in the project root. This includes test binaries, test input files, and native comparison builds.
- **Verify shim/emu availability** — check `lib/posix-shim/include/amiport/` for Tier 1 and `lib/posix-emu/include/amiport-emu/` for Tier 2 before assuming a wrapper exists
- **Follow the tier model** (ADR-008) — Tier 1 shim is automated, Tier 2 emu needs caveat documentation, Tier 3 redesign needs human review
- **Prefer shim/emu wrappers** over inline AmigaDOS rewrites — keeps ported code clean
- **Never remove functionality** — stub it with a clear message if no Amiga equivalent exists
- **Preserve original source** as reference in `original/` directory alongside the port in `ported/`
- **Document every transformation** with a comment: `/* amiport: replaced POSIX open() with amiport_open() */`
- Use `#ifdef __AMIGA__` blocks when the code should remain cross-platform
- Target **AmigaOS 3.x on 68020+** as the default. Use `#ifdef` for other OS versions.

## First-Time Setup — MANDATORY

**Run this immediately after cloning.** It configures git hooks that enforce documentation consistency and port directory hygiene. Without this, commits will not be validated.

```bash
make setup             # Configure git hooks (REQUIRED — run first)
make setup-toolchain   # Install cross-compiler Docker image
```

## Build Instructions

```bash
make help              # Show all available targets
make setup             # Configure git hooks (run after cloning)
make setup-toolchain   # Install/pull cross-compiler (Docker)
make fetch-ndk         # Download AmigaOS NDK 3.2 R4
make build-shim        # Build the POSIX shim library (Tier 1)
make build-emu         # Build the POSIX emulation library (Tier 2)
make build-console     # Build the console shim library (ncurses)
make build-net         # Build the BSD socket shim library
make build TARGET=examples/wc   # Build a specific port
make test TARGET=examples/wc    # Test via vamos
make test-shim         # Run POSIX shim library tests via vamos
make test-emu          # Run POSIX emulation library tests via vamos
make test-console      # Run console shim tests via vamos
make test-net          # Run BSD socket shim tests via vamos
make test-ports        # Test all production ports via vamos
make test-fsemu TARGET=ports/grep  # Test via FS-UAE with ARexx harness (Category 3-4)
make build-uaequit     # Build UAEQuit helper for FS-UAE test automation
make setup-debug-tools # Install Enforcer, Mungwall, SegTracker for crash debugging
make check-docs        # Validate agent references across all docs
make check-agents      # Validate agent/skill frontmatter fields
make package TARGET=examples/wc # Create LHA archive
make check-test-coverage  # Validate FS-UAE test suite completeness
make check-fix-propagation # Scan ports for known crash patterns (informational)
make clean             # Remove build artifacts
make scrape-adcd       # Scrape ADCD and regenerate reference docs (~20 min)
```

**Prerequisites:** Docker (for cross-compiler), Python + amitools (`pip install amitools`) for vamos testing.

## Toolchain

Primary: **bebbo/amiga-gcc** (`m68k-amigaos-gcc`) in Docker
Secondary: **VBCC** (`vc`) in Docker

The toolchain scripts in `toolchain/scripts/` handle detection and invocation. Always use these scripts rather than calling compilers directly.

## Testing

Use **vamos** (from amitools) for CLI program testing (Categories 1-2) — it provides a virtual AmigaOS runtime without needing a full emulator. The `test-amiga` skill handles this.

For console UI apps (Category 3), network apps (Category 4), GUI programs, or hardware-dependent code, use **FS-UAE** with a configured AmigaOS 3.x installation. Network apps additionally require a TCP/IP stack (AmiTCP or Roadshow) configured in the emulator.

### Automated FS-UAE Testing

Category 3-4 ports can be tested automatically using `make test-fsemu TARGET=ports/<name>`. This:

1. Boots FS-UAE with a custom Startup-Sequence that runs the port's ARexx test harness
2. The ARexx harness (based on `toolchain/templates/test-runner.rexx`) executes test cases defined in `test-fsemu-cases.txt`
3. Results are written in TAP format to a shared RESULTS: volume
4. UAEQuit signals FS-UAE to exit when tests complete (or a watchdog timeout fires)
5. A `TEST-REPORT.md` is generated from the TAP output

This avoids the need for manual interactive testing in the emulator. See ADR-014 for design details.

## Key References

**Critical (consult during every port):**
- `docs/posix-tiers.md` — Master POSIX tier classification (Tier 1/2/3 for every function)
- `docs/references/crash-patterns.md` — Crash pattern KB — check BEFORE fixing any bug
- `docs/references/68k-hardware.md` — 68k hardware reference — memory map, addressing modes, crash signatures, vamos differences
- `docs/references/adcd/` — Complete ADCD 2.1 in markdown — HOW to use AmigaOS functions, not just signatures
- `docs/test-coverage-standard.md` — **Mandatory** test coverage requirements (no happy-path-only testing)
- `.claude/skills/transform-source/references/transformation-rules.md` — Tier 1 transformation rules

**Architecture & guides:** `docs/architecture.md`, `docs/porting-guide.md`, `docs/api-mapping.md`

**ADRs:** `docs/adr/008` (tiers), `009` (console), `010` (bsdsocket), `011` (categories), `014` (FS-UAE testing), `015` (CI/quality), `016` (debug agent), `017` (hooks enforcement), `018` (ADCD knowledge base), `019` (agent persona matrix), `020` (git hooks validation)

**Shim references:** `docs/references/bsd-isms.md`, `docs/references/newlib-availability.md`, `docs/references/adcd/FUNCTIONS.md`, `docs/references/adcd/TYPES.md`

## Port Categories (ADR-011)

Not all ports are CLI tools. The pipeline supports five categories:

| Category | Description | Libraries Needed | Test Method |
|----------|-------------|-----------------|-------------|
| 1. CLI tools | Pure POSIX utilities | posix-shim, posix-emu | vamos |
| 2. Scripting interpreters | Lua, bc, awk | posix-shim + minor stubs | vamos |
| 3. Console UI apps | less, vim, nano | posix-shim + console-shim | FS-UAE |
| 4. Network apps | curl, wget, irc | posix-shim + bsdsocket-shim | FS-UAE + TCP/IP |
| 5. GUI apps | (future) | Intuition/MUI | FS-UAE |

When linking ported programs, use `-lamiport-console` for console UI apps and `-lamiport-net` for network apps, in addition to `-lamiport` (posix-shim).

## Known Pitfalls

Hard-won lessons from real ports (cal, diff). Read these before porting.

### fopen() macro collision
The `amiport/stdio.h` header defines `#define fopen(p, m) amiport_fopen(p, m)`. Inside `file_io.c` (which implements `amiport_fopen`), calling `fopen()` triggers infinite recursion. **Fix:** Add `#undef fopen` before the implementation. This also applies to `fclose`.

### vamos argv pointer arithmetic
POSIX programs sometimes compute argument string positions by pointer arithmetic on argv entries (e.g., `argv[argc-1] - argv[0]` to estimate total arg string length). On vamos, argv entries are NOT allocated in a contiguous block — each is independently allocated. **Fix:** Use explicit `strlen()` iteration instead of pointer subtraction between argv entries.

### printf format specifiers for Amiga types
Amiga `LONG` is `long` (32-bit), and `ULONG` is `unsigned long`. Use `%ld` / `%lu` format specifiers, not `%d` / `%u`. On 68000, `int` may be 16-bit in some compiler configurations.

### Stack size
Amiga default stack is 4KB. Most ported programs need more. Always add a stack cookie: `long __stack = 32768;` (or 65536 for recursive programs like find/diff). The value is in bytes.

### Exit codes
POSIX programs use `exit(0)` for success and `exit(1)` for failure. On AmigaOS, exit code 1 is meaningless — Amiga shells test with `IF WARN` (>=5), `IF ERROR` (>=10), `IF FAIL` (>=20). **Fix:** Replace `exit(1)` / `exit(EXIT_FAILURE)` with `exit(10)` (RETURN_ERROR). Use `exit(20)` for fatal/unrecoverable errors. The `err()`/`errx()` functions in `amiport/err.h` pass the exit code through — update those calls too (e.g., `errx(1, ...)` → `errx(10, ...)`).

### pledge/unveil stubs
OpenBSD code almost always calls `pledge()` and `unveil()`. These should be stubbed as `#define pledge(p, e) (0)` and `#define unveil(p, f) (0)` — never shimmed as functions.

### T: assign for temp files
AmigaOS has no `/tmp`. Use `T:` (which maps to `RAM:T/` by default). The `amiport_tmpfile()` and `amiport_mkstemp()` functions handle this.

### st_dev/st_ino same-file detection
Many POSIX programs compare `st_dev` and `st_ino` to detect if two paths point to the same file (e.g., `diff`'s `files_differ()`, `cp`'s hardlink detection). The stat shim now populates these from `fib_DiskKey` (st_ino) and `GetDeviceProc` (st_dev). **Check:** When porting programs that compare files, `grep` for `st_ino` or `st_dev` in the source and verify the comparison logic works with real values. See crash-patterns.md #4.

### vamos ignores __stack cookies
vamos defaults to 8KB stack and does NOT read `__stack` from the binary. Programs needing more than 8KB silently overflow into heap, then wrap SP through zero and crash in the `0xffff0000` range. The `toolchain/scripts/vamos` wrapper defaults to 128KB. For programs needing more (diff, find), pass `-s 192` explicitly. See crash-patterns.md #7.

### vsnprintf(NULL, 0, ...) crash
libnix's `vsnprintf` does NOT support `NULL` as the destination buffer (a C99 extension). Calling `vsnprintf(NULL, 0, fmt, ap)` to measure buffer size will crash on the 68000. **Fix:** Use a probe buffer: `char probe[1024]; int len = vsnprintf(probe, sizeof(probe), fmt, ap);` — vsnprintf returns the total chars that WOULD be written, even if truncated. This commonly appears when replacing `vasprintf()`/`asprintf()`. See crash-patterns.md #5.

### Epoch offset
AmigaOS epoch is 1978-01-01, Unix is 1970-01-01. The offset is 252460800 seconds (AMIGA_EPOCH_OFFSET). All time conversions must add this.

### __nowild for pattern-argument programs
Programs that accept regex or pattern arguments (grep, sed, find -name, awk) MUST define `int __nowild = 1;` at file scope. Without this, `amiport_expand_argv()` will expand pattern arguments like `*.c` before the program sees them. This matches the SAS/C convention where `__nowild` suppressed automatic argv expansion.

## Safety Hooks

The project enforces structural safety via hooks in `.claude/settings.json`:

**PreToolUse (block before execution):**
- **`block-original-edits.sh`** — Blocks Edit/Write to any path containing `/original/`. Upstream source is read-only.
- **`block-root-files.sh`** — Blocks Edit/Write of non-config files in the project root. Prevents stray build/test artifacts.
- **`block-direct-gcc.sh`** — Blocks direct `m68k-amigaos-gcc`/`ld`/`as` calls in Bash. Forces use of `make` or toolchain wrapper scripts.
- **`enforce-agents.sh`** — **Blocks direct Edit/Write to `ported/*.c` files.** Forces use of code-transformer or debug-agent. This is the hard enforcement for mandatory agent dispatch.

**Stop (completion verification):**
- **`verify-before-stop.sh`** — Reminds Claude to verify work (run tests, check for stray files, update docs) before stopping.

**PreCompact (context preservation):**
- **`save-port-context.sh`** — On auto-compaction, injects active port names into context so Claude retains awareness of in-progress work.

**SessionStart (environment checks):**
- **`check-toolchain.sh`** — Warns if Docker, vamos, lha, or jq are missing at session start.

Additionally, hookify rules block test file creation in the project root.

## Git Hooks

The repo uses `.githooks/` for git hooks, configured by `make setup` (which runs `git config core.hooksPath .githooks`):

- **commit-msg**: Enforces conventional commit prefixes (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `chore:`, `ci:`, `perf:`, `style:`, `build:`). Allows merge commits.
- **pre-commit**: Runs `make check-docs` to validate agent references, checks for stray root files, and verifies port directory hygiene. Blocks commits that would introduce doc drift or violate hygiene rules. Kept fast (<2s).
- **pre-push**: Builds the shim library and compiles all shim tests. Catches build/link breakage before it reaches origin. Runs expensive Docker cross-compilation (~10-15s).

**`make setup` is mandatory after cloning.** Without it, hook validation is skipped.

## Continuous Integration

GitHub Actions CI (`.github/workflows/ci.yml`) runs on every push to main:
- Builds posix-shim, posix-emu, console-shim, and bsdsocket-shim libraries
- Runs all shim, emulation, console, and network tests via vamos
- Validates doc consistency (`make check-docs`) and agent frontmatter (`make check-agents`)
- Builds and tests all example ports
- Builds and tests all production ports (`make build-ports`, `make test-ports`)

The toolchain Docker image is cached on GHCR (`ghcr.io/bdgscotland/amiport-toolchain:latest`).

## Shim Extension Workflow

When a port needs a POSIX function that's not yet in the shim library:

1. **Use `/extend-shim <function-name>`** — this skill automates the full process
2. It will: research the function → classify the tier → write implementation + header + test → rebuild
3. The new function becomes available for all future ports
4. Alternative: dispatch the `build-manager` agent which can diagnose "undefined reference" errors and recommend `/extend-shim`

### Adding a shim manually (if needed)

1. Check `docs/references/newlib-availability.md` — is it already in libnix?
2. Check `docs/references/bsd-isms.md` — is it a BSD-ism with a known solution?
3. Classify per `docs/posix-tiers.md` decision tree
4. Implement in `lib/posix-shim/` (Tier 1) or `lib/posix-emu/` (Tier 2)
5. Add test in `tests/shim/` or `tests/emu/`
6. Update `docs/posix-tiers.md` and `posix-to-amiga-map.md`
