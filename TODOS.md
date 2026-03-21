# TODOS

## Shim Library Enhancements

### ~~Add `amiport_dup()` and `amiport_dup2()`~~ — DONE

Implemented with parallel `fd_closeable[]` array and scan-on-close for shared BPTRs. 258-line test suite in `tests/shim/test_dup.c`. Moved from "Planned" to Tier 1 in `docs/posix-tiers.md`.

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

## Infrastructure

### Add CI/CD with GitHub Actions — IN PROGRESS

**What:** GitHub Actions workflow that runs `make smoke-test` on push to main.

**Why:** CI catches regressions automatically and proves the full pipeline works from a clean environment.

**Details:** Main complexity is running the bebbo-gcc Docker image inside GitHub Actions (Docker-in-Docker). Using GHCR for pre-built Docker image caching. Self-bootstrapping (first run builds image, subsequent runs pull cache).

**Depends on:** `make smoke-test` and `make doctor` existing and working.

---

### Agent Cost/Token Tracking

**What:** Track which agents consume the most tokens during `/port-project` runs.

**Why:** With 10 agents potentially dispatching during a single port, there's no visibility into cost distribution. Knowing which stages are expensive informs model selection decisions.

**Details:** Could be a lightweight logging mechanism in the port-project skill that records agent name, model, and approximate token count per dispatch. Output to PORT.md or a separate analytics file.

**Depends on:** Nothing — can be implemented independently.

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
