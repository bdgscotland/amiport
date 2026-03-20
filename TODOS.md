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
