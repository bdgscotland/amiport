# Test Coverage Standard

Every port MUST meet this coverage standard before it can be considered complete. Happy-path-only testing is not acceptable — AmigaOS has no memory protection, no crash recovery, and no automatic process cleanup. An untested error path can corrupt memory silently.

## Required Test Categories

### 1. Functional Tests (per flag/option)

Every documented flag and mode of operation must have at least one test case.

```
# For grep: -i, -v, -c, -n, -E, -F, -w, -o, -m, -l, -B, -A, -C
TEST: Case-insensitive match (-i)
CMD: WORK:grep -i HELLO WORK:test-input.txt
EXPECT: hello world
```

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
| 1. CLI tools | 8+ | Functional, Error, Exit Code, Edge Case |
| 2. Scripting | 10+ | Functional, Error, Exit Code, Edge Case, Amiga-Specific |
| 3. Console UI | 8+ | Functional, Error, Exit Code (+ interactive via FS-UAE) |
| 4. Network | 8+ | Functional, Error, Exit Code, Connection (+ TCP/IP via FS-UAE) |

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
- `EXPECT_CONTAINS:` — substring match (for multi-line output)
- `EXPECT_RC:` — expected Amiga return code (optional but RECOMMENDED for every test)
- Input files must be pre-created (no piping in ARexx)
- **stderr is NOT captured** — error messages from `warn()`, `err()`, `fprintf(stderr,...)` do not appear in test output. For error path tests, use `EXPECT_RC:` only. Do not use `EXPECT:` or `EXPECT_CONTAINS:` for error messages.

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

## Enforcement

The `port-project` skill MUST verify test coverage before completing Stage 5:
- Count test cases in `test-fsemu-cases.txt`
- Check for presence of EXPECT_RC assertions
- Check for error path tests (tests with EXPECT_RC: 10 or EXPECT_RC: 5)
- Reject ports with fewer than the minimum test count for their category
