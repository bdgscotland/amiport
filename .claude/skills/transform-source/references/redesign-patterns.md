# Tier 3 Redesign Patterns

Structural transformation patterns for POSIX constructs that cannot be shimmed or emulated.
These require rewriting the source code. Agents propose patterns but do NOT auto-apply them —
the port-coordinator presents options and waits for a human decision.

See [ADR-008](../../../../docs/adr/008-tiered-posix-compatibility.md) and
[docs/posix-tiers.md](../../../../docs/posix-tiers.md) for the full tier model.

---

## Pattern: fork()+exec() → SystemTags() (subprocess-and-wait)

**ID:** `subprocess-and-wait`

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

---

## Pattern: fork()+exec() → CreateNewProcTags() (async subprocess)

**ID:** `async-subprocess`

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

---

## Pattern: fork() for daemon/background → not applicable

**ID:** `daemon-fork`

**Recognise when:** Code does `fork()` with the parent exiting and the child continuing (daemon pattern).

**Transform to:** Remove the fork-to-daemon pattern entirely. On AmigaOS, CLI programs can be backgrounded with `Run >NIL:`. Add a note in the port's README that the user launches with `Run` for background operation.

---

## Pattern: pthreads → single-threaded (Option A)

**ID:** `remove-threading`

**Recognise when:** Threads are used for parallelism but the program would function correctly single-threaded (e.g., a thread pool for batch processing, protective mutexes around non-concurrent access).

**Transform to:**

```c
/* amiport-redesign: pthreads removed — single-threaded on AmigaOS */
#ifdef __AMIGA__
#define pthread_mutex_lock(m)    (0)
#define pthread_mutex_unlock(m)  (0)
#define pthread_mutex_init(m,a)  (0)
#define pthread_mutex_destroy(m) (0)

/* Replace pthread_create() with direct call */
/* Before: pthread_create(&tid, NULL, worker_func, arg); */
/* After:  worker_func(arg); */
#endif
```

**Limitations:** No parallelism. Programs that depend on concurrent execution for correctness (not just performance) need Option B.

---

## Pattern: pthreads → Exec tasks with message ports (Option B)

**ID:** `exec-tasks`

**Recognise when:** Threads communicate via shared state and the program genuinely requires concurrency (e.g., a producer-consumer pipeline, a UI thread + worker thread).

**Transform to:** Each thread becomes an Exec `Task` or `Process`. Shared state is replaced with message passing via `MsgPort` + `PutMsg()`/`GetMsg()`.

```c
/* amiport-redesign: pthread → Exec task with message port */
struct MsgPort *worker_port = CreateMsgPort();
struct Process *worker = CreateNewProcTags(
    NP_Entry,     worker_entry,
    NP_Name,      (ULONG)"worker",
    NP_StackSize, 32768,
    TAG_DONE);

/* Communication via messages instead of shared memory */
struct WorkMsg *msg = AllocVec(sizeof(struct WorkMsg), MEMF_PUBLIC);
msg->wm_Msg.mn_ReplyPort = reply_port;
msg->wm_Data = work_item;
PutMsg(worker_port, &msg->wm_Msg);
```

**Limitations:** Significant rewrite. Requires understanding the program's concurrency architecture. Message passing has higher latency than shared memory.

---

## Pattern: BSD sockets → bsdsocket.library

**ID:** `bsdsocket`

**Recognise when:** Code uses `socket()`, `connect()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`, and includes `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`.

**Transform to:**

```c
/* amiport-redesign: BSD sockets → bsdsocket.library */
#include <proto/socket.h>
#include <bsdsocket/socketbasetags.h>

struct Library *SocketBase = NULL;

/* At startup: */
SocketBase = OpenLibrary("bsdsocket.library", 4);
if (!SocketBase) {
    fprintf(stderr, "TCP/IP stack not available\n");
    return RETURN_FAIL;
}

/* Replace close(sockfd) → CloseSocket(sockfd) */
/* Replace ioctl(sockfd, ...) → IoctlSocket(sockfd, ...) */
/* Replace select() on sockets → WaitSelect() */

/* At cleanup: */
CloseLibrary(SocketBase);
```

**Limitations:** Requires a TCP/IP stack to be installed (AmiTCP, Roadshow, or Miami). Not all Amiga users will have one. Port should detect missing bsdsocket.library gracefully.

---

## Pattern: mmap(MAP_SHARED) → explicit file I/O

**ID:** `mmap-shared-to-fileio`

**Recognise when:** Code uses `mmap()` with `MAP_SHARED` and `PROT_WRITE` to modify a file through a memory mapping, or uses mmap for IPC between processes.

**Transform to:** Replace with explicit `Read()`/`Write()`/`Seek()` calls. If the code uses the mapping as a random-access data structure, introduce a buffer layer that reads/writes sections on demand. If used for IPC, replace with message ports or shared memory via `AllocMem(MEMF_PUBLIC)`.

---

## Pattern: Unix permissions → Amiga protection bits (or ignore)

**ID:** `unix-permissions`

**Recognise when:** Code calls `chmod()`, `chown()`, checks `st_mode` for `S_IRUSR`/`S_IWUSR`/etc., or uses permission bits in `open()` with `O_CREAT`.

**Transform to:** For most ports, make these no-ops. Amiga protection bits (FIBF_READ, FIBF_WRITE, FIBF_EXECUTE, FIBF_DELETE) have inverted semantics (set = DENIED) and no user/group/other ownership model.

```c
/* amiport-redesign: chmod() is a no-op — Amiga protection bits differ */
#ifdef __AMIGA__
#define chmod(path, mode) (0)
#define chown(path, uid, gid) (0)
#endif
```

---

## Pattern: /proc and /sys filesystem access → system library calls

**ID:** `proc-sys-fs`

**Recognise when:** Code reads from `/proc/self/...`, `/proc/cpuinfo`, `/sys/class/...`, or similar pseudo-filesystem paths.

**Transform to:** Replace with direct Exec/DOS library queries:
- Process info → `FindTask(NULL)`
- System info → `exec.library` data structures
- Hardware info → generally not queryable; stub or remove

---

## Agent instructions

When the analysis skill classifies a function as `needs-redesign` (Tier 3):

1. Identify which pattern applies from this document
2. Include the pattern ID in the analysis report
3. The port-coordinator will present the pattern with its limitations to the user
4. Do NOT auto-apply — wait for human confirmation
5. If no pattern matches, describe the issue and recommend manual review
