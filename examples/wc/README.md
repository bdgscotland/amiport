# wc — Word Count (amiport MVP)

This is the first amiport example: a BSD-style `wc` utility ported to AmigaOS 3.x.

## Files

- `original/wc.c` — Original POSIX source
- `ported/wc.c` — Amiga-compatible source (transformed by amiport)
- `Makefile` — Cross-compilation build file

## Changes Made

The port required only minimal changes:

1. `#include <unistd.h>` replaced with `#include <amiport/unistd.h>`
2. `#include <getopt.h>` replaced with `#include <amiport/getopt.h>`
3. Added Amiga version string (`$VER:`)
4. Added stack size cookie (`__stack = 32768`)

All other code (stdio, string, ctype) works unchanged via clib2.

## Build

```bash
# From the project root:
make build TARGET=examples/wc

# Or from this directory:
make
```

## Test

```bash
make test
# Requires vamos (pip install amitools)
```
