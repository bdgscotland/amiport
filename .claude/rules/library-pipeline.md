Paths: lib/**/*, tests/**/*

# Library Pipeline — Same Discipline as Ports

**Bundled libraries in `lib/<name>/` are first-class pipeline targets. Use the same agent discipline as `/port-project` applies to `ports/<name>/`.**

The `/port-project` skill and its mandatory-agent gates are framed around
`ports/<name>/`, but the same pipeline stages must be applied when porting
a bundled dependency to `lib/<name>/` (zlib, oniguruma, libgit2, mbedTLS,
or any future library port). Do NOT treat libraries as a shortcut.

## Why this rule exists

**2026-04-12, porting zlib 1.3.1 to `lib/zlib/`:**

1. First action after copying upstream source was `make -C lib/zlib` to see
   if it compiled — no source-analyzer, no KB query, no agent dispatch
2. Test suite was hand-written (`tests/zlib/test_zlib.c`) before
   test-designer was dispatched — bypassed the test-coverage-standard audit

User corrected on both: *"Use your kb and pipeline tooling"* and *"you should
have used the test designer."* Manual shortcuts produce inferior output: the
agents run stub-value impact analysis, canonical-reference lookups,
crash-pattern matching, and coverage-category enforcement that inline work
skips.

## Mandatory Stages for `lib/<name>/` Ports

Apply in order. Do NOT skip stages. Do NOT run `make` before Stage 1.

### Stage 0: KB query (MANDATORY)

Before touching any file, query the amiga-kb MCP:

```
amiga_pitfalls_for  { topic: "<library> <domain>" }
amiga_search        { query: "<library> port AmigaOS 68k" }
```

Capture any hits and factor them into the analysis — existing pitfalls
may tell you which `-D` defines you need, which files to exclude, or
which bebbo-gcc codegen quirks to work around up front.

### Stage 1: Source-analyzer agent (MANDATORY)

Dispatch `source-analyzer` with **library-mode framing**:

> "Portability audit of library source tree at `lib/<name>/src/`. This is
> a library port (not a CLI tool) — verdict is whether downstream ports
> can safely link against `lib<name>.a`. Look for POSIX surface area,
> endianness assumptions, alignment hazards (crash-patterns #15),
> large locals (crash-patterns #10), struct-by-value returns
> (crash-patterns #16), non-ASCII bytes, and any 68k-specific issues.
> Do not rewrite source — audit only."

**GATE:** Do NOT build until source-analyzer returns with a verdict
(CLEAN / CAVEATS / INFEASIBLE). The audit output tells you which `-D`
defines the build needs, which files to include/exclude, and whether any
source requires code-transformer before the first build.

### Stage 2: Directory setup + Makefile

Create `lib/<name>/src/`, `lib/<name>/include/`, copy upstream source,
hand-write the Makefile modeled on `lib/oniguruma/Makefile` or
`lib/zlib/Makefile`:

- `-O0 -noixemul -m68000` as base flags (per known-pitfalls:
  default bundled libraries to `-O0` until proven safe)
- `-m68000` is mandatory — vamos emulates 68000; libraries with 68020+
  instructions crash tests with `ALERT: code=00068020`
- `TOOLCHAIN_BIN = ../../toolchain/scripts` for the wrapper scripts

### Stage 3: Build (direct `make` allowed)

`make -C lib/<name>/` is **explicitly allowed** — the
`warn-direct-port-build.sh` hook exempts library builds. This is the one
stage where a pipeline agent is NOT mandatory. Iterate on errors:

- Missing header → add the right `-D` define, not a workaround
- Implicit declaration warning → add the right `-D` define (e.g.,
  `-DHAVE_UNISTD_H` for zlib, same pattern as other POSIX-feature probes)
- Struct-return corruption at `-O1`/`-O2` → stay at `-O0`
  (crash-patterns #16)
- 68020 instruction emitted → check that `-m68000` is actually on the
  command line; use the `TOOLCHAIN_BIN` wrapper scripts

If the build hits non-trivial errors (missing symbol, incompatible type,
preprocessor conflict), dispatch `build-manager` for recovery — same as
for port builds.

### Stage 4: Test-designer agent (MANDATORY)

Dispatch `test-designer` with **library unit test framing**:

> "Design a comprehensive library unit test plan for `lib/<name>/`. The
> target artifact is a C source file (`tests/<name>/test_<name>.c`) using
> `tests/shim/test_framework.h`'s `TEST(name) { ASSERT(...); }` macros,
> built by `tests/<name>/Makefile`, run via vamos. This is NOT an FS-UAE
> `test-fsemu-cases.txt` suite — there is no `EXPECT_RC:` harness for
> library code. Audit the API surface at `lib/<name>/include/` and the
> source at `lib/<name>/src/` against `docs/test-coverage-standard.md`.
> Cover all six categories: functional (per API call), error path (each
> return code), edge case (boundary values), Amiga-specific (68k endian,
> alignment, stack pressure, z_off64_t limits if applicable), and
> stress (large buffers within the 256 KB stack cap). Return a prioritized
> test plan — I'll implement the C code."

**This stage is mandatory even though there is no FS-UAE test suite.**
The test-designer still performs API surface mapping, return-code
enumeration, and coverage-category enforcement that manual test authoring
skips.

### Stage 5: Implement tests + test-runner agent (MANDATORY)

Implement the test-designer's plan as `tests/<name>/test_<name>.c` and
`tests/<name>/Makefile`. Then dispatch `test-runner` to run via vamos.

**Required Makefile elements for library tests:**
- `VAMOS_STACK = 256` (or more) — vamos default 8 KB is insufficient for
  libraries with large stream buffers. Pass `-s $(VAMOS_STACK)` to vamos.
- `long __stack = 262144;` cookie in `test_<name>.c` — real AmigaOS reads
  this (vamos ignores it, hence the `-s` flag).
- `-I../shim` to pick up `test_framework.h`
- `-L../../lib/<name> -l<name>` for linking

**GATE:** Do not proceed to Stage 6 until all tests pass.

### Stage 6: Memory-checker agent (MANDATORY)

Dispatch `memory-checker` with library-mode framing:

> "Memory safety audit of `lib/<name>/src/`. This is a library — scope is
> whether `lib<name>.a` is safe to link against. Look for caller-
> responsibility leaks (is ownership documented?), incomplete error-path
> cleanup (partial allocation on init failure), custom allocator hook
> safety (NULL callbacks handled?), file handle leaks, double-free / UAF,
> unsafe realloc patterns. Do NOT modify upstream source — report only."

AmigaOS has no memory protection and no process memory cleanup on exit
with `-noixemul`. Library-side leaks are permanent.

### Stage 7: Perf-optimizer agent (MANDATORY)

Dispatch `perf-optimizer` with **68k-specific framing**:

> "68k performance review of `lib/<name>/`. Identify hot loops on the
> 68000 @ 7 MHz target. Evaluate the `-O0` default vs the cost of codegen
> bugs at `-O1`/`-O2`. Identify safe candidates for per-file `-O1`
> promotion (scalar-only files with no struct-by-value returns >8 bytes
> — crash-patterns #16). Recommend `-D` defines that avoid software
> divides, enable inline-worthy paths, or skip dead-code paths gated on
> other architectures."

**Apply HIGH/CRITICAL findings immediately** — do not defer. Per-file
`-O1` promotion for audited hot-path files is the pattern (see
`lib/zlib/Makefile` for the reference implementation: `HOTPATH_CFLAGS`
variable + per-file rules for the scalar-return hot-path files).

### Stage 8: Rebuild + re-test after optimizations (MANDATORY)

If Stage 6 or Stage 7 made changes, **rebuild and re-run the test suite**.
Re-run memory-checker and perf-optimizer on the updated code until no
CRITICAL or HIGH findings remain.

### Stage 9: Docs + top-level Makefile (MANDATORY)

- `CLAUDE.md` codebase map entry for `lib/<name>/`
- `README.md` compat-libs table row with library name, purpose, link flag
- Top-level `Makefile`:
  - Add `build-<name>` target → `$(MAKE) -C lib/<name>`
  - Add `test-<name>` target → `$(MAKE) -C tests/<name>` (depends on `build-<name>`)
  - Add both to `.PHONY`
  - Add both to the `help:` output
- Run `make check-docs` to verify doc consistency

## Forbidden Shortcuts

- **Do not `make -C lib/<name>/` before Stage 1 (source-analyzer).** The
  build succeeding does not prove portability. It means the code compiles
  — not that it is safe to link, not that it avoids endian hazards, not
  that optimizations are safe to enable.
- **Do not hand-write tests before Stage 4 (test-designer).** You will
  miss API surface, error paths, and coverage categories that the agent
  catches. The amiport test-coverage-standard is not optional for
  libraries.
- **Do not defer memory-checker or perf-optimizer** with "it's upstream
  code, it's already been audited." Upstream zlib / libgit2 / oniguruma
  have been audited on x86/ARM/Linux — not on bebbo-gcc/libnix/68k. The
  amiport environment has unique constraints (no GC, no process cleanup,
  `-m68000`, `-O0` codegen risks) that general-purpose audits don't catch.
- **Do not proceed to another port that depends on this library** (e.g.,
  starting libgit2 before zlib is fully pipeline-audited) until all
  mandatory stages have returned CLEAN.

## Reference implementations

- `lib/oniguruma/` — minimal Makefile pattern, -O0 default, ASCII-only build
- `lib/zlib/` — per-file `-O1` hot-path promotion (`HOTPATH_CFLAGS` pattern),
  `-DNO_DIVIDE` style perf defines, ANSI-only build with libnix native fd
- `tests/zlib/Makefile` — `VAMOS_STACK = 256` + `-s` flag pattern
- `tests/zlib/test_zlib.c` — `__stack` cookie, `test_framework.h` usage

## See also

- `.claude/rules/use-pipeline-agents.md` — the parent rule for port pipelines
- `.claude/rules/amiga-coding.md` — C coding standards (applies to library
  source too via its `Paths:` filter, which already includes `lib/**/*.c`)
- `docs/test-coverage-standard.md` — the coverage categories test-designer
  enforces
- `docs/pdr/010-amigit-on-libgit2.md` — the downstream consumer that makes
  the library pipeline discipline load-bearing for Phase 2 libgit2
