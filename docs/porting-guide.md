# Porting Guide

## Quick Start

The fastest way to port a C program to the Amiga:

```bash
# In Claude Code, from the amiport directory:
/port-project /path/to/your/source.c
```

This runs the full pipeline: analyze, transform, build, test, package.

## Manual Pipeline

For more control, run each stage individually:

### 1. Analyze

```bash
/analyze-source /path/to/source/
```

Produces a portability report identifying:
- POSIX headers that need replacement
- System calls that need shimming
- Blocking patterns (fork, mmap, pthreads) that require redesign
- Severity classification (trivial / needs-shim / blocking)

### 2. Transform

```bash
/transform-source /path/to/source/
```

Creates a `ported/` directory alongside the original with:
- Headers swapped for amiport shim equivalents
- Function calls replaced with `amiport_*` wrappers
- Blocking patterns stubbed with clear messages
- Amiga boilerplate added (version string, stack cookie)

### 3. Build

```bash
/build-amiga
```

Cross-compiles the ported source using the Docker toolchain. Handles:
- Compiler flag selection based on target profile
- Linking with posix-shim library
- Iterative error fixing

### 4. Test

```bash
/test-amiga
```

Runs the compiled binary in vamos and verifies output.

### 5. Package

Built into the build skill — creates an LHA archive with the binary and documentation.

## What Can Be Ported

**Good candidates** (high success rate):
- CLI utilities (wc, cat, head, tail, sort, uniq, cut, tr)
- Text processors (sed-like tools, simple parsers)
- Math/science tools (calculators, unit converters)
- Games with text/simple graphics
- Compression tools (with bundled algorithm implementations)

**Harder but possible**:
- Programs using ncurses (map to Amiga console device)
- Programs with simple networking (requires bsdsocket.library)
- Larger applications with many source files (needs agent teams)

**Not currently supported**:
- GUI applications (GTK, Qt, X11)
- Programs requiring fork/exec process model
- Programs depending on shared memory (mmap, shm)
- Multithreaded programs (pthreads)
- Programs with x86 inline assembly

## Tips for Successful Ports

1. **Start small**: Port the simplest version of a program first
2. **Check dependencies**: Programs with few external library dependencies port easiest
3. **Read the analysis report**: It tells you exactly what's blocking
4. **Test incrementally**: Build and test after each transformation, not just at the end
5. **Stack size**: Set `__stack` to at least 32768 — many ported programs need more stack than the Amiga default
