---
name: build-amiga
description: Cross-compile C source for Amiga using bebbo-gcc or VBCC in Docker. Manages compiler flags, linking against posix-shim, and packaging. Use after source transformation.
allowed-tools: Bash, Read, Write, Edit, Grep, Glob
---

# Build for Amiga

You are cross-compiling transformed C source code for AmigaOS using the Docker-based toolchain.

## Process

1. **Detect toolchain** — run `toolchain/scripts/detect-toolchain.sh` to find available compilers
2. **Select target profile** — default is `amiga-68020.mk` (AmigaOS 3.x, 68020+)
3. **Build posix-shim** if not already built — `make build-shim`
4. **Compile** the source using the appropriate cross-compiler
5. **Link** against `libamiport.a` (the posix-shim library)
6. **Handle errors** — if compilation fails, analyze the error, fix the source, and retry
7. **Package** — create LHA archive with binary and documentation

## Compiler Flags

### bebbo-gcc (primary)
```bash
m68k-amigaos-gcc \
  -O2 \
  -noixemul \
  -m68020 \
  -I../../lib/posix-shim/include \
  -L../../lib/posix-shim \
  -o program \
  source.c \
  -lamiport
```

### VBCC (secondary)
```bash
vc +kick13 \
  -O2 \
  -I../../lib/posix-shim/include \
  -L../../lib/posix-shim \
  -o program \
  source.c \
  -lamiport
```

## Key Flags

- `-noixemul` — Use clib2/newlib instead of ixemul (Unix emulation layer). We provide our own shim.
- `-m68020` — Target 68020 CPU (default). Change per target profile.
- `-I` / `-L` — Point to posix-shim headers and library

## Error Handling

When compilation fails:
1. Read the full error output
2. Identify whether it's a missing header, undefined function, type mismatch, or linker error
3. For missing functions: check if the posix-shim needs to be extended
4. For type errors: check transformation rules for the correct replacement
5. Fix the source and retry
6. Maximum 5 retry cycles before reporting failure

## Packaging

After successful build:
```bash
# Create LHA archive
lha a program.lha program program.readme
```

The `.readme` file follows Aminet format:
```
Short:    One-line description
Uploader: amiport (automated)
Author:   Original author
Type:     util/cli
Architecture: m68k-amigaos >= 3.0
```
