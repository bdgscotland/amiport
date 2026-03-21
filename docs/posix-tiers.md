# POSIX Compatibility Tiers

Classification guide for POSIX functions in the amiport pipeline. Each function is assigned to a tier based on how closely AmigaOS can replicate its semantics. Agents use this document during analysis and transformation.

See [ADR-008](adr/008-tiered-posix-compatibility.md) for the decision record.

## Tier Summary

|Tier|Label    |Library          |Agent action                      |Colour|Severity                 |
|----|---------|-----------------|----------------------------------|------|-------------------------|
|1   |Shim     |`lib/posix-shim/`|Automated transform               |Green |`trivial` or `needs-shim`|
|2   |Emulation|`lib/posix-emu/` |Transform + document caveats      |Yellow|`needs-emu`              |
|3   |Redesign |Pattern templates|Propose options, wait for decision|Red   |`needs-redesign`         |

-----

## Tier 1 — Shim (Direct Mapping)

One wrapper per function. Semantics match POSIX for all common use cases. The transform-source skill applies these mechanically.

### Implemented

|POSIX function  |Shim wrapper            |AmigaDOS call               |Notes                                                            |
|----------------|------------------------|----------------------------|-----------------------------------------------------------------|
|`open()`        |`amiport_open()`        |`Open()`                    |Flag mapping: O_RDONLY→MODE_OLDFILE, O_CREAT|O_TRUNC→MODE_NEWFILE|
|`close()`       |`amiport_close()`       |`Close()`                   |Protects fd 0/1/2                                                |
|`read()`        |`amiport_read()`        |`Read()`                    |                                                                 |
|`write()`       |`amiport_write()`       |`Write()`                   |                                                                 |
|`lseek()`       |`amiport_lseek()`       |`Seek()`                    |Handles Amiga returning old position                             |
|`stat()`        |`amiport_stat()`        |`Lock()`+`Examine()`        |Maps DateStamp to Unix timestamp                                 |
|`fstat()`       |`amiport_fstat()`       |`ExamineFH()`               |Requires AmigaOS 2.0+                                            |
|`opendir()`     |`amiport_opendir()`     |`Lock()`+`Examine()`        |                                                                 |
|`readdir()`     |`amiport_readdir()`     |`ExNext()`                  |                                                                 |
|`closedir()`    |`amiport_closedir()`    |`UnLock()`                  |                                                                 |
|`mkdir()`       |`amiport_mkdir()`       |`CreateDir()`               |Mode bits ignored                                                |
|`getcwd()`      |`amiport_getcwd()`      |`GetCurrentDirName()`       |                                                                 |
|`chdir()`       |`amiport_chdir()`       |`CurrentDir()` + `Lock()`   |                                                                 |
|`unlink()`      |`amiport_unlink()`      |`DeleteFile()`              |                                                                 |
|`rename()`      |`amiport_rename()`      |`Rename()`                  |                                                                 |
|`access()`      |`amiport_access()`      |`Lock()` test               |Mode bits ignored (no Unix perms)                                |
|`sleep()`       |`amiport_sleep()`       |`Delay()`                   |Granularity: 1/50s ticks                                         |
|`usleep()`      |`amiport_usleep()`      |`Delay()`                   |Rounds up to nearest tick                                        |
|`getenv()`      |`amiport_getenv()`      |`GetVar()`                  |Returns malloc'd string                                          |
|`getpid()`      |`amiport_getpid()`      |`FindTask(NULL)`            |Returns task address as int                                      |
|`isatty()`      |`amiport_isatty()`      |`IsInteractive()`           |                                                                 |
|`time()`        |`amiport_time()`        |`DateStamp()` + epoch offset|                                                                 |
|`gettimeofday()`|`amiport_gettimeofday()`|`DateStamp()`               |Microsecond field is approximate                                 |
|`getopt()`      |`amiport_getopt()`      |Pure C implementation       |                                                                 |
|`strtok_r()`    |`amiport_strtok_r()`    |Pure C implementation       |                                                                 |
|`signal(SIGINT)`|`amiport_signal()`      |`SetSignal()` / Ctrl-C      |Only SIGINT is meaningful                                        |
|`raise(SIGINT)` |`amiport_raise()`       |`Signal()`                  |                                                                 |
|`tmpfile()`     |`amiport_tmpfile()`     |`Open("T:...")`             |Uses T: assign                                                   |

|`fnmatch()`    |`amiport_fnmatch()`     |Pure C implementation       |Shell-style glob matching                                        |
|`scandir()`    |`amiport_scandir()`     |`opendir()`+`readdir()`     |With filter and sort callbacks                                   |
|`alphasort()`  |`amiport_alphasort()`   |`strcmp()` wrapper           |For use with `scandir()`                                         |
|`strlcpy()`    |`amiport_strlcpy()`     |Pure C implementation       |BSD safe string copy                                             |
|`strlcat()`    |`amiport_strlcat()`     |Pure C implementation       |BSD safe string concatenation                                    |
|`reallocarray()`|`amiport_reallocarray()`|`realloc()` with overflow check|OpenBSD safe array reallocation                               |
|`asprintf()`   |`amiport_asprintf()`    |`vsnprintf()`+`malloc()`    |Dynamic string formatting                                        |
|`vasprintf()`  |`amiport_vasprintf()`   |`vsnprintf()`+`malloc()`    |va_list variant of asprintf                                      |
|`mkstemp()`    |`amiport_mkstemp()`     |`Open()` with unique name   |Uses task address + counter for uniqueness                       |
|`pread()`      |`amiport_pread()`       |`Seek()`+`Read()`+`Seek()`  |Non-atomic positional read                                       |
|`pwrite()`     |`amiport_pwrite()`      |`Seek()`+`Write()`+`Seek()` |Non-atomic positional write                                      |
|`getline()`    |`amiport_getline()`     |`fgets()`+`realloc()`       |GNU extension; fgets-based for 68k perf                          |
|`getdelim()`   |`amiport_getdelim()`    |`fgets()`/`fgetc()`+`realloc()`|Read until delimiter; fast path for '\n'                      |

### Planned Tier 1 additions

|POSIX function    |Implementation approach      |Priority                                           |
|------------------|-----------------------------|---------------------------------------------------|
|`dup()` / `dup2()`|fd_table ref-counting        |High — needed for I/O redirection                  |
|`chmod()`         |No-op with comment           |Low — Amiga has protection bits but different model|
|`rmdir()`         |`DeleteFile()`               |Low — same as unlink on Amiga                      |
|`readlink()`      |`ReadLink()` (OS 2.0+)       |Low — soft links rare on classic Amiga             |
|`symlink()`       |`MakeLink()` (OS 2.0+)       |Low                                                |
|`ftruncate()`     |`SetFileSize()` (OS 2.0+)    |Medium — needed by some editors                    |
|`fileno()`        |Lookup in amiport_files table|Medium — already partially implemented             |

-----

## Tier 2 — Emulation (Approximate Mapping)

These require stateful emulation layers that approximate POSIX behaviour with documented divergence. Each lives in `lib/posix-emu/` with its own source file and header. Ported code linking against Tier 2 functions gets `/* amiport-emu: ... */` comments explaining the differences.

### select() / poll() — I/O multiplexing

**POSIX contract:** Wait for one or more file descriptors to become ready for I/O.

**Emulation strategy:** `WaitForChar()` polling loop with `Wait()` on a combined signal mask. For single-fd cases, `WaitForChar(fh, timeout)` is a direct match. For multi-fd, the emulation layer maintains a mapping from amiport fds to signal bits and polls in round-robin with configurable granularity.

**Behavioural differences:**

- No true level-triggered readiness notification — uses polling
- Granularity limited to ~20ms (1 tick) minimum
- Does not work with socket fds (those need bsdsocket.library's `WaitSelect()`)
- `exceptfds` parameter is ignored

**When to use:** Programs doing interactive terminal I/O with timeouts. Programs multiplexing reads from multiple files (less common on Amiga). Simple event loops.

**When NOT to use:** Programs where select() is the core event loop for networked I/O — these need Tier 3 redesign to use bsdsocket.library directly.

### mmap(MAP_PRIVATE, PROT_READ) — read-only file mapping

**POSIX contract:** Map a file into memory for read access.

**Emulation strategy:** `AllocMem()` a buffer of `st_size` bytes, `Read()` the entire file into it, return the pointer. `munmap()` calls `FreeMem()`.

**Behavioural differences:**

- Entire file is read into memory upfront (no lazy page faulting)
- Memory usage equals file size (no page sharing, no copy-on-write)
- File changes after mmap are not visible (snapshot semantics only)
- `MAP_SHARED` with write-back is NOT supported — use Tier 3

**When to use:** Programs that mmap a config file or data file for read access. Programs that mmap for convenience rather than performance.

**When NOT to use:** Programs mapping gigabyte files (Amiga memory is scarce). Programs relying on MAP_SHARED write-back. Programs using mmap for IPC.

### pipe() — basic pipe emulation

**POSIX contract:** Create a unidirectional data channel with read and write ends.

**Emulation strategy:** Open `PIPE:amiport_NNNN` (AmigaOS 2.0+ PIPE: device) for both read and write. Assign fds via the fd_table.

**Behavioural differences:**

- Named pipe, not anonymous — visible in the filesystem namespace
- Blocking/buffering behaviour differs from POSIX anonymous pipes
- EOF detection is less reliable
- No `SIGPIPE` on write to closed read end

**When to use:** Programs using pipe() for simple parent→child data passing where the child is launched via `SystemTags()`.

**When NOT to use:** Programs building complex pipe chains. Programs relying on pipe semantics for synchronisation.

### alarm() / setitimer() — timer signals

**POSIX contract:** Deliver SIGALRM after a timeout.

**Emulation strategy:** `timer.device` request with a callback that sets a signal bit. `amiport_check_alarm()` checks the signal in the main loop (cooperative, not pre-emptive).

**Behavioural differences:**

- Not asynchronous — requires cooperative checking
- No signal delivery interrupting blocking calls
- Resolution limited to timer.device granularity (~1ms on 68020+, ~20ms on 68000)

**When to use:** Programs using alarm() for simple timeouts where the main loop can check periodically.

**When NOT to use:** Programs relying on SIGALRM to interrupt blocking read()/select() calls.

### regex — POSIX regular expressions

**POSIX contract:** Compile and execute regular expressions (BRE and ERE).

**Emulation strategy:** A minimal backtracking NFA regex engine compiled into `lib/posix-emu/`. Supports literal matching, `.`, `^`, `$`, `[]`, `[^]`, `*`, `+`, `?`, `|`, `()`, `\1`-`\9` backreferences, and common escapes (`\t`, `\n`).

**Behavioural differences:**

- No locale-dependent collation (`[.ch.]`, `[=a=]`)
- No POSIX character classes (`[:alpha:]`, `[:digit:]`)
- Maximum pattern length: 512 bytes compiled
- Maximum 9 capture groups
- Backtracking NFA — not suitable for adversarial patterns (catastrophic backtracking possible)
- `REG_NEWLINE` flag only partially supported

**When to use:** Programs using regex for simple pattern matching (grep, diff `-I`, sed basics).

**When NOT to use:** Programs relying on full POSIX locale support or running untrusted regex patterns.

### Tier 2 design rules

1. Each emulation module is a single `.c`/`.h` pair in `lib/posix-emu/`
2. Headers are `<amiport-emu/select.h>`, `<amiport-emu/mmap.h>`, etc.
3. Functions are prefixed `amiport_emu_` to distinguish from Tier 1 shims
4. Every header has a `/* EMULATION NOTICE */` comment block listing behavioural differences
5. Emulation modules may depend on Tier 1 shim internals (fd_table access) via a private API header `<amiport/internal.h>` — never the reverse
6. Each module includes a `AMIPORT_EMU_xxx_ENABLED` compile-time flag so ports can opt out

-----

## Tier 3 — Redesign (Paradigm Mismatch)

No library can bridge these. The source code must be structurally rewritten. The agent pipeline proposes patterns but does not auto-apply them.

### Pattern: fork()+exec() → SystemTags() (subprocess-and-wait)

**Recognise when:** Code does `fork()`, the child immediately calls `exec*()`, and the parent calls `waitpid()`. This is the "run a subprocess and collect its exit status" pattern.

**Transform to:**

```c
/* amiport-redesign: fork+exec+waitpid → SystemTags() */
LONG rc = SystemTags(command_string,
    SYS_Input,  Input(),
    SYS_Output, Output(),
    NP_StackSize, 32768,
    TAG_DONE);
/* rc is the AmigaDOS return code (RETURN_OK, RETURN_WARN, etc.) */
```

**Limitations:** No separate PID. No async execution (parent blocks). No pipe to child stdin/stdout without extra setup.

### Pattern: fork()+exec() → CreateNewProcTags() (async subprocess)

**Recognise when:** Code does `fork()`, the child calls `exec*()`, and the parent does NOT immediately `waitpid()` — it continues doing other work, possibly checking later.

**Transform to:**

```c
/* amiport-redesign: fork+exec (async) → CreateNewProcTags() */
struct Process *child = CreateNewProcTags(
    NP_Entry,     child_entry_func,
    NP_Name,      (ULONG)"child_task",
    NP_StackSize, 32768,
    NP_Input,     Open("NIL:", MODE_OLDFILE),
    NP_Output,    Open("NIL:", MODE_NEWFILE),
    NP_CloseInput,  TRUE,
    NP_CloseOutput, TRUE,
    TAG_DONE);
```

**Limitations:** The child must be an Amiga entry point function, not an external program. For external programs, use `SystemTags()` from within the child process. No shared memory between parent and child. Synchronisation requires explicit message passing via Exec message ports.

### Pattern: fork() for daemon/background → not applicable

**Recognise when:** Code does `fork()` with the parent exiting and the child continuing (daemon pattern).

**Transform to:** On AmigaOS this is the default — CLI programs can be `Run >NIL:` to background. The fork-to-daemon pattern should be removed entirely, with a note in the README that the user launches with `Run`.

### Pattern: pthreads → single-threaded or explicit Exec tasks

**Option A — Remove threading (preferred for simple cases):**
**Recognise when:** Threads are used for parallelism but the program would function correctly single-threaded (e.g., a thread pool for batch processing).

**Transform to:** Remove all pthread calls. Replace mutex lock/unlock with no-ops. Replace `pthread_create()` with a direct function call. Replace `pthread_join()` with nothing.

**Option B — Exec tasks with message ports (complex cases):**
**Recognise when:** Threads communicate via shared state and the program genuinely requires concurrency (e.g., a producer-consumer pipeline, a UI thread + worker thread).

**Transform to:** Each thread becomes an Exec `Task` or `Process`. Shared state is replaced with message passing via `MsgPort` + `PutMsg()`/`GetMsg()`. This is a significant rewrite and requires understanding the program's concurrency architecture.

### Pattern: BSD sockets → bsdsocket-shim library

**Recognise when:** Code uses `socket()`, `connect()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`, and includes `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`.

**Transform to:** Link against `lib/bsdsocket-shim/` (ADR-010). The shim wraps bsdsocket.library (AmiTCP/Roadshow/Miami) with automatic lifecycle management:

- Replace headers: `<sys/socket.h>` → `<amiport-net/socket.h>`, `<netinet/in.h>` → `<amiport-net/netinet/in.h>`, etc.
- Replace `socket()` → `amiport_socket()`, `connect()` → `amiport_connect()`, etc.
- Replace `close(sockfd)` → `amiport_closesocket()` (routes to CloseSocket automatically)
- Replace `select()` on sockets → `amiport_net_select()` (uses WaitSelect, handles mixed file/socket)
- Replace `gethostbyname()` → `amiport_gethostbyname()`
- Library open/close is automatic (first use → atexit cleanup)

See `docs/references/bsdsocket-mapping.md` for the full API mapping table.

**Limitations:** Requires a TCP/IP stack to be installed and running. No IPv6. No SSL/TLS (requires AmiSSL separately). Graceful degradation returns ENOTSUP when no stack installed.

### Pattern: ncurses/termcap → console-shim library

**Recognise when:** Code uses `initscr()`, `endwin()`, `getch()`, `addstr()`, `mvprintw()`, etc. and includes `<curses.h>` or `<ncurses.h>`.

**Transform to:** Link against `lib/console-shim/` (ADR-009). The shim maps ncurses calls to ANSI escape sequences on Amiga console.device:

- Replace headers: `<curses.h>` / `<ncurses.h>` → `<amiport-console/curses.h>`
- Most ncurses functions work unchanged (initscr, endwin, move, addch, getch, attron, etc.)
- Input uses RAW: console mode for character-at-a-time reading
- Colors map to standard 8 ANSI colors

See `docs/references/console-ansi-mapping.md` for escape sequence mapping.

**Limitations:** No mouse support. No wide characters. No alternate screen buffer. Line drawing uses ASCII fallbacks. Window size detection limited. Tier 2 functions (getch, newwin) have caveats.

### Pattern: mmap(MAP_SHARED) → explicit file I/O

**Recognise when:** Code uses `mmap()` with `MAP_SHARED` and `PROT_WRITE` to modify a file through a memory mapping, or uses mmap for IPC between processes via a shared file.

**Transform to:** Replace with explicit `Read()`/`Write()`/`Seek()` calls. If the code uses the mapping as a random-access data structure, introduce a buffer layer that reads/writes sections on demand. If used for IPC, replace with message ports or shared memory via `AllocMem(MEMF_PUBLIC)`.

### Pattern: Unix permissions → Amiga protection bits (or ignore)

**Recognise when:** Code calls `chmod()`, `chown()`, checks `st_mode` for `S_IRUSR`/`S_IWUSR`/etc., or uses permission bits in `open()` with `O_CREAT`.

**Transform to:** For most ports, make these no-ops. Amiga has protection bits (FIBF_READ, FIBF_WRITE, FIBF_EXECUTE, FIBF_DELETE and group/other variants) but the semantics are inverted (set bit = permission DENIED on classic AmigaOS) and there's no concept of user/group/other ownership in the Unix sense. If the program's security model genuinely depends on file permissions, it needs architectural rethinking — but most ported CLI tools just need mode bits removed from `open()` calls and `chmod()` stubbed.

### Pattern: /proc and /sys filesystem access → system library calls

**Recognise when:** Code reads from `/proc/self/...`, `/proc/cpuinfo`, `/sys/class/...`, or similar pseudo-filesystem paths.

**Transform to:** Replace with direct Exec/DOS library queries. Process info via `FindTask(NULL)`. System info via `exec.library` data structures. Hardware info is generally not queryable in a portable way on AmigaOS — stub or remove.

-----

## Decision tree for agents

When the analysis skill encounters a POSIX function not already classified:

```
1. Does AmigaDOS have a function that does the same thing
   with the same calling convention?
   → YES: Tier 1 (shim)
   → MOSTLY: Tier 1 with documented edge cases
   → NO: continue

2. Can the behaviour be emulated with a stateful wrapper
   that handles the common use case correctly?
   → YES, and the common case works: Tier 2 (emulation)
   → YES, but only niche cases work: Tier 3 (redesign)
   → NO: continue

3. Does the function represent a fundamentally different
   execution model (processes, threads, virtual memory,
   filesystem semantics)?
   → YES: Tier 3 (redesign)
   → The function is entirely OS-specific with no Amiga
     equivalent at all: stub with warning, classify as
     Tier 3
```

-----

## Analysis report format

The analysis skill should classify each issue with its tier:

```json
{
  "function": "select",
  "file": "main.c",
  "line": 42,
  "tier": 2,
  "severity": "needs-emu",
  "context": "Used for stdin timeout in interactive mode",
  "recommendation": "Single-fd select on stdin — amiport_emu_select() handles this case well",
  "caveats": ["20ms granularity minimum", "exceptfds ignored"]
}
```

```json
{
  "function": "fork",
  "file": "process.c",
  "line": 118,
  "tier": 3,
  "severity": "needs-redesign",
  "context": "fork+exec+waitpid pattern — runs external command and collects exit code",
  "pattern": "subprocess-and-wait",
  "recommendation": "Replace with SystemTags()",
  "requires_human_review": true
}
```
