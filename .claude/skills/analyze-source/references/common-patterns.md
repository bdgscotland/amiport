# Common Porting Patterns

Frequently encountered code patterns in Linux/POSIX C programs and how to handle them for Amiga.

## Pattern 1: Command-Line Argument Parsing

**Frequency**: Nearly universal in CLI tools

```c
// POSIX pattern:
#include <getopt.h>
while ((c = getopt(argc, argv, "hvn:")) != -1) { ... }
```

**Amiga solution**: Use bundled `amiport_getopt()` from posix-shim. Drop-in replacement.

**Severity**: needs-shim (trivial effort)

---

## Pattern 2: File Processing Loop

**Frequency**: Very common (cat, wc, grep, sort, etc.)

```c
// POSIX pattern:
FILE *fp = fopen(filename, "r");
while (fgets(line, sizeof(line), fp)) { ... }
fclose(fp);
```

**Amiga solution**: No change needed — `stdio.h` functions are provided by clib2/newlib.

**Severity**: trivial

---

## Pattern 3: stdin/stdout/stderr

**Frequency**: Universal

```c
// POSIX pattern:
fprintf(stderr, "error: %s\n", msg);
while ((c = getchar()) != EOF) { ... }
```

**Amiga solution**: No change needed — C runtime provides these.

**Severity**: trivial

---

## Pattern 4: Directory Traversal

**Frequency**: Common in file utilities (find, du, ls)

```c
// POSIX pattern:
DIR *d = opendir(path);
struct dirent *entry;
while ((entry = readdir(d))) {
    if (entry->d_type == DT_DIR) { ... }
}
closedir(d);
```

**Amiga solution**: Use `amiport_opendir()` / `amiport_readdir()` / `amiport_closedir()`. Note: `d_type` is not standard POSIX — use `amiport_stat()` or check `fib_DirEntryType` directly.

**Severity**: needs-shim

---

## Pattern 5: Error Handling with errno

**Frequency**: Very common

```c
// POSIX pattern:
if (open(file, O_RDONLY) < 0) {
    perror("open");
    exit(EXIT_FAILURE);
}
```

**Amiga solution**: `amiport_open()` sets errno via the errno mapping shim. `perror()` works via clib2. Main difference: underlying errors come from `IoErr()` and are mapped to closest POSIX errno.

**Severity**: needs-shim (transparent to calling code)

---

## Pattern 6: Temp Files

**Frequency**: Moderate (sort, editors, etc.)

```c
// POSIX pattern:
char *tmp = tmpnam(NULL);  // or mkstemp()
FILE *f = fopen(tmp, "w");
```

**Amiga solution**: Use `T:` assign instead of `/tmp/`. Replace hardcoded `/tmp/` paths with `T:`.

```c
// Amiga:
FILE *f = fopen("T:amiport_tmp_XXXXXX", "w");
```

**Severity**: needs-shim

---

## Pattern 7: Path Handling

**Frequency**: Common

```c
// POSIX pattern:
snprintf(path, sizeof(path), "%s/%s", dir, file);
if (path[0] == '/') { /* absolute */ }
```

**Amiga solution**: Amiga uses `/` for path separators (same as Unix!) but volumes use `:` prefix. Absolute paths start with a volume name (`SYS:`, `Work:`, `RAM:`), not `/`.

Key replacements:
- `/dev/null` → `NIL:`
- `/tmp/` → `T:`
- `/home/` → `S:` or no equivalent
- Check for leading `/` → check for `:` in path

**Severity**: needs-shim (path translation helper)

---

## Pattern 8: Process Spawning

**Frequency**: Moderate (shells, make, package managers)

```c
// POSIX pattern:
pid_t pid = fork();
if (pid == 0) {
    execvp(program, args);
}
waitpid(pid, &status, 0);
```

**Amiga solution**: **BLOCKING** — no fork/exec on Amiga. For simple "run a command" cases, use `SystemTags()`:

```c
// Amiga equivalent (simple case only):
LONG result = SystemTags(command_string,
    SYS_Output, Output(),
    SYS_Input, Input(),
    TAG_DONE);
```

This only works for running external commands, not for the fork/exec pattern of parent-child parallelism.

**Severity**: blocking

---

## Pattern 9: Signal Handling

**Frequency**: Moderate (long-running programs, servers)

```c
// POSIX pattern:
signal(SIGINT, handler);
signal(SIGTERM, handler);
signal(SIGPIPE, SIG_IGN);
```

**Amiga solution**: Only SIGINT (Ctrl-C) has a meaningful mapping via `SIGBREAKF_CTRL_C`. Other signals are stubbed. The shim provides `amiport_signal()` that:
- Maps `SIGINT` → Ctrl-C checking via `SetSignal()`
- Ignores `SIGPIPE`, `SIGTERM`, `SIGHUP` (no-op)
- Returns `SIG_ERR` for others

**Severity**: needs-shim (partial functionality only)

---

## Pattern 10: Environment Variables

**Frequency**: Common

```c
// POSIX pattern:
char *home = getenv("HOME");
char *path = getenv("PATH");
```

**Amiga solution**: Use `amiport_getenv()` which calls `GetVar()`. Note: Amiga environment variables are stored differently (ENV: assign for global, local vars per shell). Some standard vars don't exist — `HOME` has no Amiga equivalent (use `SYS:` or `PROGDIR:`).

**Severity**: needs-shim

---

## Pattern 9: Wildcard/Glob Arguments

**Frequency**: Common in CLI tools that process files

Programs that accept filename arguments from the command line expect the shell to
have already expanded wildcards (`*.c` → `foo.c bar.c`). AmigaOS shells do NOT
expand wildcards — they pass them literally.

**Detection**: Check if the program's main() processes argv entries as filenames
(opens, reads, or stats them). If so, it needs argv expansion.

**Amiga solution**: Add `amiport_expand_argv(&argc, &argv)` at top of `main()` and
`amiport_free_argv()` before `_exit()`. Include `<amiport/glob.h>`.

**Exception**: Programs that accept regex/pattern arguments (grep, sed, awk, find)
need `int __nowild = 1;` at file scope to prevent expansion of pattern args.
Without this, `grep *.c file.txt` would expand `*.c` before grep sees it.

**Severity**: needs-shim
