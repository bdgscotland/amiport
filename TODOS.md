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

**Details:** AmigaOS 2.0+ has PIPE: device which can be opened as a regular file. A basic implementation would open "PIPE:amiport_NNN" for read and write ends. However, AmigaOS PIPE: is not exactly POSIX pipes — it's a named pipe device, not anonymous. Behavior differences around blocking, buffering, and EOF detection make this complex to get right. Most CLI tools being ported (wc, grep, cat, etc.) don't use pipes internally — the shell handles piping between programs. Lower priority than dup/dup2.

**Depends on:** Would benefit from dup/dup2 for full pipe+redirect support.

---

## Infrastructure

### Add CI/CD with GitHub Actions

**What:** GitHub Actions workflow that runs `make smoke-test` on push to main.

**Why:** The smoke test currently only runs locally. CI catches regressions automatically and proves the full pipeline (toolchain setup → shim build → example build → vamos test) works from a clean environment.

**Details:** Main complexity is running the bebbo-gcc Docker image inside GitHub Actions (Docker-in-Docker). May need a self-hosted runner or a pre-built Docker image cached in GHCR. The workflow should: install vamos via pip, pull/build the cross-compiler Docker image, then run `make smoke-test`. Estimated ~30 min with CC.

**Priority:** P2 — ship after the golden thread (smoke test + examples) is complete.

**Depends on:** `make smoke-test` and `make doctor` existing and working.
