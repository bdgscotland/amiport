# TODOS

## Shim Library Enhancements

### Add `amiport_dup()` and `amiport_dup2()`

**What:** Implement file descriptor duplication using the existing fd_table.

**Why:** `dup2()` is used in many C programs for I/O redirection (e.g., redirecting stderr to a file). Without it, any program that does I/O redirection is unportable.

**Details:** The fd_table in `file_io.c` maps int→BPTR. Adding dup requires a reference count per BPTR to avoid double-Close when two fds share the same file handle. Estimated ~50 lines of implementation + tests.

**Depends on:** Should be tested with the vamos-based test suite.

---

### Add `amiport_pipe()` for basic pipe emulation

**What:** Implement minimal `pipe()` using AmigaDOS PIPE: device (available on AmigaOS 2.0+).

**Why:** Pipes are used in many Unix programs for inter-process communication. Even simple programs use `popen()`/`pclose()` for running subcommands.

**Details:** AmigaOS 2.0+ has PIPE: device which can be opened as a regular file. A basic implementation would open "PIPE:amiport_NNN" for read and write ends. However, AmigaOS PIPE: is not exactly POSIX pipes — it's a named pipe device, not anonymous. Behavior differences around blocking, buffering, and EOF detection make this complex to get right. Most CLI tools being ported don't use pipes internally — the shell handles piping between programs. Lower priority than dup/dup2.

**Depends on:** Would benefit from dup/dup2 for full pipe+redirect support.

---

### Amiga wildcard/glob expansion shim

**What:** `amiport_glob_args()` to expand `#?` and `*` patterns in argv at program startup.

**Why:** Amiga shells don't glob-expand like Unix. Programs ported from Unix expect the shell to expand wildcards, but AmigaOS passes them literally. Support both Amiga (`#?`, `~`, `(a|b)`) and Unix (`*`, `?`, `[a-z]`) patterns.

---

### Amiga path translation shim

**What:** `amiport_path_translate()` for `/tmp` → `T:`, `/dev/null` → `NIL:`, `~/` → `HOME:`.

**Why:** Common Unix path patterns break ports. An automatic translation layer at fopen/open time would fix this transparently.

---

## FS-UAE Testing

### FS-UAE serial console debug mode

**What:** When FS-UAE tests fail, re-run with `--debug` to get a live TCP connection to an Amiga shell inside the emulator.

**Why:** Interactive debugging of test failures without leaving the terminal. FS-UAE can expose the serial port as TCP via `serial_port = tcp://127.0.0.1:1234/wait`.

**Details:** Requires setting up the AUX: DOSDriver on the Amiga side and running `newshell AUX:` to get a shell over serial. Output parsing is fragile (ANSI escapes, prompts mixed with output). Lower priority than the automated pipeline.

**Depends on:** FS-UAE automated testing pipeline (completed).

---

## Infrastructure

### Add CI/CD with GitHub Actions

**What:** GitHub Actions workflow that runs `make smoke-test` on push to main.

**Why:** CI catches regressions automatically and proves the full pipeline works from a clean environment.

**Details:** Main complexity is running the bebbo-gcc Docker image inside GitHub Actions (Docker-in-Docker). May need a self-hosted runner or a pre-built Docker image cached in GHCR.

**Depends on:** `make smoke-test` and `make doctor` existing and working.

---

### Verify newlib-availability.md

**What:** Audit `docs/references/newlib-availability.md` against current bebbo-gcc libnix.

**Why:** bebbo has been cross-porting functions between libnix and newlib; our reference may be stale.

---

## Port Targets

### Category 1 (CLI) — Next Candidates

| Tool | Aminet Status | Complexity | Notes |
|------|--------------|------------|-------|
| sort | v1.0 (1993) | Easy | Pure computation, locale stubs |
| tr | missing | Trivial | Character translation |
| uniq | missing | Trivial | Line comparison |
| tail | missing | Easy | File I/O + seek |
| tee | missing | Trivial | stdin → file + stdout |
| xargs | missing | Easy | Needs glob shim |
| basename/dirname | missing | Trivial | String manipulation |

### Category 2 (Scripting) — Lua 5.4

See ADR-011. Needs dlopen stubs and path remapping for `require()`.

### Category 3 (Console UI) — less

Console-shim library complete (ADR-009). `less` is the first target. Needs window size detection via Intuition.

### Category 4 (Network) — wget-lite

BSD socket shim complete (ADR-010). Needs a simple HTTP client to validate.
