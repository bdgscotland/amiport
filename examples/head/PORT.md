# Port Report: head

## Original
- **Program**: head (print first N lines of files)
- **Source**: Custom implementation for amiport examples
- **Language**: C89
- **POSIX APIs used**: open(), read(), close(), write(), getopt()

## Analysis
- **Verdict**: EASY
- All POSIX calls have direct amiport shim equivalents
- No blocking patterns (no fork, threads, sockets, mmap)

## Transformations Applied

| # | File:Line | Original | Replacement | Type |
|---|-----------|----------|-------------|------|
| 1 | head.c:includes | `<unistd.h>` | `<amiport/unistd.h>` | Header |
| 2 | head.c:includes | `<getopt.h>` | `<amiport/getopt.h>` | Header |
| 3 | head.c:main | `open()` | `amiport_open()` | Function call |
| 4 | head.c:main | `read()` | `amiport_read()` | Function call |
| 5 | head.c:main | `close()` | `amiport_close()` | Function call |
| 6 | head.c:main | `write()` | `amiport_write()` | Function call |
| 7 | head.c:main | `getopt()` | `amiport_getopt()` | Function call |
| 8 | head.c:top | — | Version string added | Boilerplate |
| 9 | head.c:top | — | Stack cookie added | Boilerplate |

## Shim Functions Exercised
- `amiport_open()` -- file opening with O_RDONLY
- `amiport_read()` -- buffered reading
- `amiport_close()` -- file closing
- `amiport_write()` -- output to stdout/stderr via fd 1/2
- `amiport_getopt()` -- command-line option parsing

## Build
```
make          # Cross-compile with m68k-amigaos-gcc
make test     # Run tests via vamos
make clean    # Remove build artifacts
```

## Test Results
- Default 10 lines: PASS
- Custom line count (-n): PASS
- Empty file: PASS
- Missing file: PASS (error message printed)
- Stdin reading: Supported but requires piped input in vamos

## Known Limitations
- No `-c` (byte count) option -- not implemented in this example
- Stdin mode requires piped input; vamos doesn't support interactive terminal input well
