# AmigaOS API Quick Reference

Key AmigaOS libraries and functions used by the amiport shim and transformation rules.

## dos.library (DOS Functions)

The primary library for file I/O, directory operations, and process management.

### File Operations

```c
#include <proto/dos.h>

/* Open a file. Returns BPTR filehandle or ZERO on failure. */
BPTR Open(CONST_STRPTR name, LONG accessMode);
/* accessMode: MODE_OLDFILE (read), MODE_NEWFILE (write/create), MODE_READWRITE (r/w) */

/* Close a file */
BOOL Close(BPTR file);

/* Read from file. Returns bytes read, 0 for EOF, -1 for error */
LONG Read(BPTR file, APTR buffer, LONG length);

/* Write to file. Returns bytes written, -1 for error */
LONG Write(BPTR file, CONST_APTR buffer, LONG length);

/* Seek in file. Returns OLD position. */
LONG Seek(BPTR file, LONG position, LONG mode);
/* mode: OFFSET_BEGINNING, OFFSET_CURRENT, OFFSET_END */

/* Delete a file or empty directory */
BOOL DeleteFile(CONST_STRPTR name);

/* Rename a file */
BOOL Rename(CONST_STRPTR oldName, CONST_STRPTR newName);

/* Get error code for last DOS operation */
LONG IoErr(void);

/* Get file info from filehandle (dos.library 36+, OS 2.0+) */
BOOL ExamineFH(BPTR fh, struct FileInfoBlock *fib);
```

### Directory Operations

```c
/* Lock a file/directory for examination */
BPTR Lock(CONST_STRPTR name, LONG type);
/* type: SHARED_LOCK (read), EXCLUSIVE_LOCK (write) */

/* Release a lock */
void UnLock(BPTR lock);

/* Examine a locked file/directory — fills FileInfoBlock */
BOOL Examine(BPTR lock, struct FileInfoBlock *fib);

/* Get next entry in directory (after Examine) */
BOOL ExNext(BPTR lock, struct FileInfoBlock *fib);

/* Create a directory. Returns lock on new dir (must UnLock!) */
BPTR CreateDir(CONST_STRPTR name);

/* Get name from a lock (dos.library 36+) */
BOOL NameFromLock(BPTR lock, STRPTR buffer, LONG len);

/* Get current directory name (dos.library 36+) */
BOOL GetCurrentDirName(STRPTR buf, LONG len);

/* Set current directory. Returns old lock. */
BPTR CurrentDir(BPTR lock);
```

### Process/Environment

```c
/* Run a command string (dos.library 36+) */
LONG SystemTags(CONST_STRPTR command, ULONG tag1, ...);

/* Get/Set environment variables (dos.library 36+) */
LONG GetVar(CONST_STRPTR name, STRPTR buffer, LONG size, LONG flags);
BOOL SetVar(CONST_STRPTR name, CONST_STRPTR buffer, LONG size, LONG flags);
/* flags: GVF_GLOBAL_ONLY, GVF_LOCAL_ONLY, LV_VAR */

/* Delay execution (in 1/50s ticks) */
void Delay(LONG ticks);
/* Example: Delay(50) = 1 second on PAL */
```

### FileInfoBlock Structure

```c
struct FileInfoBlock {
    LONG   fib_DiskKey;
    LONG   fib_DirEntryType;  /* >0 = directory, <0 = file */
    char   fib_FileName[108]; /* null-terminated filename */
    LONG   fib_Protection;    /* protection bits (RWED etc.) */
    LONG   fib_EntryType;
    LONG   fib_Size;          /* file size in bytes */
    LONG   fib_NumBlocks;
    struct DateStamp fib_Date;
    char   fib_Comment[80];
    /* ... more fields */
};
```

## exec.library (Executive Functions)

Core system library for tasks, memory, signals, and message passing.

### Memory

```c
#include <proto/exec.h>

/* Allocate memory with size tracking */
APTR AllocVec(ULONG byteSize, ULONG requirements);
/* requirements: MEMF_PUBLIC, MEMF_CHIP, MEMF_FAST, MEMF_CLEAR */

/* Free AllocVec'd memory */
void FreeVec(APTR memoryBlock);

/* Allocate memory (must remember size yourself) */
APTR AllocMem(ULONG byteSize, ULONG requirements);

/* Free AllocMem'd memory */
void FreeMem(APTR memoryBlock, ULONG byteSize);
```

### Signals

```c
/* Get current task */
struct Task *FindTask(CONST_STRPTR name);
/* FindTask(NULL) returns the current task */

/* Set signal flags. Returns old signals. */
ULONG SetSignal(ULONG newSignals, ULONG signalMask);

/* Send signal to a task */
void Signal(struct Task *task, ULONG signalSet);

/* Wait for signals */
ULONG Wait(ULONG signalSet);

/* Predefined break signals */
#define SIGBREAKF_CTRL_C  (1L << 12)  /* maps to Unix SIGINT */
#define SIGBREAKF_CTRL_D  (1L << 13)
#define SIGBREAKF_CTRL_E  (1L << 14)
#define SIGBREAKF_CTRL_F  (1L << 15)
```

### Checking for Ctrl-C (SIGINT equivalent)

```c
/* Check if user pressed Ctrl-C without clearing the signal */
if (SetSignal(0L, 0L) & SIGBREAKF_CTRL_C) {
    /* Ctrl-C was pressed */
    SetSignal(0L, SIGBREAKF_CTRL_C); /* Clear it */
}

/* Or use the dos.library convenience: */
if (CheckSignal(SIGBREAKF_CTRL_C)) {
    /* Ctrl-C was pressed (also clears the signal) */
}
```

## utility.library

```c
#include <proto/utility.h>

/* Tag-based argument passing (used extensively in OS 2.0+) */
struct TagItem {
    ULONG ti_Tag;
    ULONG ti_Data;
};
#define TAG_DONE  0
#define TAG_END   0
```

## Common Amiga Types

```c
typedef long           LONG;    /* 32-bit signed */
typedef unsigned long  ULONG;   /* 32-bit unsigned */
typedef short          WORD;    /* 16-bit signed */
typedef unsigned short UWORD;   /* 16-bit unsigned */
typedef char           BYTE;    /* 8-bit signed */
typedef unsigned char  UBYTE;   /* 8-bit unsigned */
typedef void          *APTR;    /* Generic pointer */
typedef long           BPTR;    /* BCPL pointer (actual_addr >> 2) */
typedef char          *STRPTR;  /* String pointer */
typedef long           BOOL;    /* Boolean (TRUE/FALSE) */
#define TRUE  1
#define FALSE 0
```

## Important Notes

- **BPTR**: Amiga file handles are BCPL pointers (address divided by 4). The `BADDR()` macro converts BPTR→APTR. Never dereference a BPTR directly.
- **Error handling**: No errno. Use `IoErr()` after failed dos.library calls to get error code. Use `PrintFault()` for human-readable error messages.
- **Stack**: Default stack is often 4KB. Ported programs should set `LONG __stack = 32768;` or higher.
- **Path separators**: Amiga uses `/` like Unix, but volumes use `:` (e.g., `SYS:C/Dir`). No leading `/` for absolute paths — use volume names instead.
- **Case sensitivity**: AmigaOS filesystem (FFS/OFS) is case-insensitive but case-preserving.
