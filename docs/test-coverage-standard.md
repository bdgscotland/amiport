# Test Coverage Standard

Every port MUST meet this coverage standard before it can be considered complete. Happy-path-only testing is not acceptable — AmigaOS has no memory protection, no crash recovery, and no automatic process cleanup. An untested error path can corrupt memory silently.

## Required Test Categories

### 1. Functional Tests (per flag/option)

**Every single flag/option accepted by the program** must have at least one test case. Not "most flags" — ALL of them. Extract the flag list from getopt() or the OPTIONS string in the source, and verify each one appears in at least one CMD: line.

The test-designer MUST cross-reference the OPTIONS string (e.g., `"0123456789abC:cdD:efhI:iL:nNPpqrS:sTtU:uwX:x:"`) against the test cases and flag any untested flags.

```
# For grep: -i, -v, -c, -n, -E, -F, -w, -o, -m, -l, -B, -A, -C
TEST: Case-insensitive match (-i)
CMD: WORK:grep -i HELLO WORK:test-input.txt
EXPECT: hello world
```

### 1a. Positional Argument Matrix (per command)

**Every command must be tested across its full positional-argument matrix, not just its flag matrix.** A flag-complete test suite that omits positional variations is still incomplete. This is the "100% of args" rule, complementary to the "100% of flags" rule in section 1.

For each command (or for each subcommand of a dispatching binary like `amigit <subcmd>`), enumerate and test:

1. **Zero positional args** — if the command has any default behavior (e.g., `init` defaults to CWD, `log` walks from HEAD, `ls` lists CWD, `cat` reads stdin), the zero-arg form MUST be tested. Not from the FS-UAE default directory — from a realistic CWD established via an Execute-script wrapper (see "Testing CWD-dependent commands" below).
2. **Exactly one positional arg** — the explicit form (e.g., `init <path>`, `log <ref>`, `grep <pattern>`).
3. **Multiple positional args** — if the command accepts variadic input (e.g., `add file1 file2 file3`, `cat a b c`, `diff file1 file2`).
4. **Nonexistent / invalid positional arg** — what happens when the target doesn't exist, is the wrong type, or is malformed.
5. **Zero args combined with flags** — flag + default behavior (e.g., `init --bare` from a CWD, not `init --bare <path>`; `log -n 5` with HEAD default).
6. **Relative vs absolute paths** — when a command accepts paths, test both absolute (`WORK:foo`) and relative-to-CWD (`foo` after `CD`) forms. These hit different libnix/libgit2 code paths and can fail independently.

Omitting zero-arg or relative-path forms is the single most common blind spot in amiport test design: if every test uses an explicit `T:path-here`, the "user runs the tool in their own project directory" case never gets exercised, and CWD-resolution bugs (NameFromLock quirks, libgit2 `.`-handling, libnix `getcwd()` behavior) ship undetected. This was the root-cause gap behind the amigit 0.1 CWD-init regression (2026-04-13): 81/81 tests green, but `amigit init` from a user CWD had never been invoked.

#### Testing CWD-dependent commands

**For any command whose default behavior depends on the current directory, the test suite MUST include at least one invocation run from inside a real CWD via an Execute-script wrapper.** The ARexx `ADDRESS COMMAND` path cannot persist `CD` across calls — each `CMD:` line starts a fresh shell context. The only reliable pattern is an ARexx wrapper that writes a small Execute script containing `CD <path>` + `<program> [args]` + `$RC` capture, then `Execute`s it. Reference implementations:

- `ports/amigit/test-amigit-inrepo.rexx` — runs a subcommand inside a pre-built repo fixture
- `ports/amigit/test-amigit-cwd-init.rexx` — runs `amigit init` (no args) from a freshly created CWD

The test-designer MUST add a "Positional Argument Matrix" subsection to its coverage report listing every command and, for each cell of the matrix above, whether the cell is covered, deferred (with reason), or not applicable.

### 2. Error Path Tests

Every error condition the program can report must be tested:

```
# Nonexistent file
TEST: Error on nonexistent file
CMD: WORK:grep pattern WORK:nonexistent.txt
EXPECT_CONTAINS: No such file
EXPECT_RC: 10

# Bad arguments
TEST: Error on missing pattern
CMD: WORK:grep
EXPECT_RC: 10

# Invalid option
TEST: Error on invalid flag
CMD: WORK:grep -Z pattern WORK:test-input.txt
EXPECT_RC: 10
```

### 3. Exit Code Tests

Every distinct exit code the program can return must be tested:

```
# Success (RETURN_OK = 0)
TEST: Match found returns 0
CMD: WORK:grep hello WORK:test-input.txt
EXPECT_RC: 0

# Warning (RETURN_WARN = 5) — e.g., grep no match, diff files differ
TEST: No match returns RETURN_WARN
CMD: WORK:grep NONEXISTENT WORK:test-input.txt
EXPECT_RC: 5

# Error (RETURN_ERROR = 10) — e.g., bad args, file not found
TEST: Bad args returns RETURN_ERROR
CMD: WORK:grep
EXPECT_RC: 10
```

### 4. Edge Case Tests

Boundary conditions specific to each tool:

```
# Empty file
TEST: Empty file produces no output
CMD: WORK:grep pattern WORK:test-empty.txt
EXPECT:
EXPECT_RC: 5

# Very long line (tests buffer handling)
TEST: Long line matches
CMD: WORK:grep pattern WORK:test-longline.txt
EXPECT_CONTAINS: pattern

# Binary/special characters
TEST: Binary file detected
CMD: WORK:grep pattern WORK:test-binary.dat
EXPECT_CONTAINS: Binary file
```

### 5. Amiga-Specific Tests

Tests that verify AmigaOS-specific behavior:

```
# Amiga path handling
TEST: Amiga volume path works
CMD: WORK:grep hello WORK:test-input.txt
EXPECT: hello world

# T: temp file handling (if applicable)
TEST: Temp files use T:
CMD: WORK:lua -e "print(os.tmpname():sub(1,2))"
EXPECT: T:
```

## Minimum Coverage Per Port Category

| Category | Min FS-UAE Tests | Required Categories |
|----------|-----------------|---------------------|
| 1. CLI tools | 15+ | Functional, Error, Exit Code, Edge Case, Amiga-Specific |
| 2. Scripting | 20+ | Functional, Error, Exit Code, Edge Case, Amiga-Specific |
| 3. Console UI | 12+ (+ 3 ITEST + 3 SCRAPE) | Functional, Error, Exit Code, Interactive (ITEST: via KeyInject, ADR-023), Visual (SCRAPE: ADR-024) |
| 4. Network | 12+ (+ 3 ITEST + 3 SCRAPE) | Functional, Error, Exit Code, Connection, Interactive (ITEST: via KeyInject), Visual (SCRAPE: ADR-024) |

### Depth Requirements (not just breadth)

The test-designer should go **deeper** on each category, not just check the box:

- **Functional:** Test every documented flag AND common flag combinations (e.g., `-r -n`, `-f -u`). At least 2 combination tests.
- **Error paths:** Test EVERY error message in the source (grep for `err(`, `errx(`, `fprintf(stderr`). Not just "bad option" and "missing file" — test permission errors, malformed input, invalid flag values, conflicting options.
- **Edge cases:** Include at minimum: empty file, single-line file, very long line (>1000 chars), file with no trailing newline, file with only whitespace, special characters in filenames (spaces, colons). Create dedicated test input files for each.
- **Amiga-specific:** Test Amiga volume paths (WORK:, T:, RAM:), test with AmigaDOS path separators, test output to Amiga-specific locations.
- **Regression:** If crash-patterns.md has entries relevant to this tool, add a test for each.

## Test Case File Format

```
TEST: description (what behavior is being tested)
CMD: WORK:program args WORK:inputfile.txt
EXPECT: expected first-line output (exact match)
EXPECT_RC: expected Amiga return code (0, 5, 10, or 20)

TEST: substring assertion example
CMD: WORK:program -u WORK:input.txt
EXPECT_CONTAINS: substring to find in output
EXPECT_RC: 0
```

- `EXPECT:` — exact match of first line of stdout (empty = no output expected)
- `EXPECT_LINE: N,text` — exact match of line N (1-indexed) of stdout. Use for multi-line output verification.
- `EXPECT_CONTAINS:` — substring match (for multi-line output). **Use only when exact match is not possible** (non-deterministic output, variable line positions).
- `EXPECT_RC:` — expected Amiga return code (optional but RECOMMENDED for every test)
- Input files must be pre-created (no piping in ARexx)
- **stderr is NOT captured** — error messages from `warn()`, `err()`, `fprintf(stderr,...)` do not appear in test output. For error path tests, use `EXPECT_RC:` only. Do not use `EXPECT:` or `EXPECT_CONTAINS:` for error messages.

### Output Verification Requirements

1. **Every `EXPECT:` and `EXPECT_LINE:` value must be derived from running the native tool**, not guessed or hand-crafted. The test-designer must run the native macOS tool to compute exact expected output.
2. **Prefer `EXPECT:` over `EXPECT_CONTAINS:`** for deterministic output. `EXPECT_CONTAINS:` is a last resort for non-deterministic or position-variable output.
3. **Multi-line output programs must verify beyond line 1.** Use `EXPECT_LINE:` to check at least one non-first line. For N-line output, verify line 1 (EXPECT:) and line N or a late line (EXPECT_LINE:).
4. **Spacing/alignment programs (expand, comm, cut) must use exact `EXPECT:`**, never `EXPECT_CONTAINS:` for column verification.

### 6. Visual Verification Tests (Category 3+ — ADR-024)

**Category 3 and 4 ports MUST have visual verification tests** in a separate `test-fsemu-visual-cases.txt` file. These verify screen content using `SCRAPE` + `EXPECT_AT` directives:

```
ITEST: Visual: file content appears on screen
LAUNCH: WORK:less WORK:test-file.txt
KEYS: WAIT2000,q
SCRAPE
EXPECT_AT 1,1,First line of file content
EXPECT_RC: 0
```

**Functional and visual tests MUST be separate FS-UAE passes.** Never mix `SCRAPE` tests in `test-fsemu-cases.txt`. Resource exhaustion at ~13 ITESTs is a hard wall. Run visual tests with `make test-fsemu TARGET=ports/<name> VISUAL=1`.

Minimum visual tests for Category 3+:
- **Content display** — verify file/data content renders on screen
- **Status/mode line** — verify program status bar renders correctly
- **Clean exit** — verify screen restores after quit (no garbage)

**Current limitation:** `CMD_WRITE` captures static display (file load, help text) but NOT interactive echo (typed characters, cursor movement). Interactive rendering verification is deferred to ADR-025.

## Stress Test Limits

**Stress test input files MUST NOT exceed 500 lines.** On 68k hardware at 7MHz, files larger than ~500 lines cause FS-UAE test timeouts (600s limit). A 10,000-line file that processes in <1s natively can take 10+ minutes on emulated 68000, and any Guru Meditation on a large file blocks the entire test harness.

- Maximum stress file: **500 lines**
- Maximum line length for stress tests: **320 characters**
- If the original source's stress tests use larger inputs, reduce them and document why

This was discovered during batch porting (2026-03-26) when 10,000-line stress files caused colrm and unexpand to timeout on FS-UAE.

## Test Input Files

Each port must include pre-created test input files for FS-UAE testing:

```
ports/<name>/
├── test-<name>-input.txt      # Standard test input
├── test-<name>-empty.txt      # Empty file (0 bytes)
├── test-<name>-*.txt           # Additional test-specific inputs
└── test-fsemu-cases.txt        # Test case definitions
```

## Deriving Test Cases

When creating tests for a ported tool, use these sources:

1. **Man page** — every flag documented in the man page needs a test
2. **Upstream test suite** — check if the original project has tests (e.g., OpenBSD regress tests, GNU test suite). Port the most relevant cases.
3. **Error messages** — grep the source for `err(`, `errx(`, `warn(`, `fprintf(stderr` — each error message is an error path that should be tested
4. **Exit codes** — grep for `exit(` and `return` in `main()` — each distinct exit code needs a test
5. **Known pitfalls** — check `docs/references/crash-patterns.md` for crash patterns that apply to this tool
6. **Positional argument matrix** — for every command (or subcommand), enumerate the cells in section 1a (zero args, one arg, multiple args, invalid arg, zero-args-plus-flag, relative-path-from-CWD) BEFORE writing test cases, then verify each cell is covered. This is mandatory. Extract positional-arg handling from the source by grepping for `argc`, `argv[N]`, and looking at optional vs required arg flows in each command's dispatch.

## Enforcement

The `port-project` skill MUST verify test coverage before completing Stage 5:
- Count test cases in `test-fsemu-cases.txt`
- Check for presence of EXPECT_RC assertions
- Check for error path tests (tests with EXPECT_RC: 10 or EXPECT_RC: 5)
- Reject ports with fewer than the minimum test count for their category
- For Category 3+: verify `test-fsemu-visual-cases.txt` exists with >= 3 SCRAPE tests
- **Require a Positional Argument Matrix section in the test-designer's coverage report**. If the matrix is missing or has uncovered cells without a documented deferral reason, Stage 5 is not complete.
- **For any command that accepts zero positional args with default CWD behavior, require at least one test that runs the command inside a CWD via an Execute-script wrapper** (grep `test-fsemu-cases.txt` for `CD ` inside Execute scripts, or look for a `test-*-cwd-*.rexx` / `test-*-inrepo*.rexx` helper).
- Verify no SCRAPE tests are in `test-fsemu-cases.txt` (they must be in the visual file)
