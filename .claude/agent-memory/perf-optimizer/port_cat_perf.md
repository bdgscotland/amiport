---
name: port_cat_perf
description: Performance findings for ports/cat 1.34 — cook_buf getc per-byte hotloop, BUFSIZ 512 too small for raw_cat reviewed 2026-03-26
type: project
---

# Performance Findings: ports/cat 1.34

**Reviewed:** 2026-03-26

## Hot Paths

1. **raw_cat() (cat.c:253-283)** — Default path (no flags). Block read/write with malloc'd buffer at BUFSIZ. Dominant path for `cat file`.
2. **cook_buf() (cat.c:186-250)** — Active when -b, -e, -n, -s, -t, or -v flags are used. Character-at-a-time processing with getc()/putchar().

## Key Findings

### HIGH: BUFSIZ (512 bytes) too small for raw_cat disk throughput (cat.c:268)

```c
bsize = BUFSIZ;
if ((buf = malloc(bsize)) == NULL)
    err(10, NULL);
```

libnix BUFSIZ is 512 bytes. Amiga disk DMA operates in 512-byte sectors; an optimal buffer is a multiple of 512, but 512 itself means one AmigaDOS Read() call per sector with maximum per-call overhead. On a 7MHz A500 with OFS, each Read() call has ~200-400 cycle overhead from dos.library dispatch. For a 10KB file that is 20 Read() calls at 512 bytes vs 3 calls at 4096 bytes.

**Fix:** Use a fixed 4096-byte buffer:
```c
bsize = 4096;
```
Or better, define a port-local macro:
```c
#define CAT_BUFSIZ 4096
bsize = CAT_BUFSIZ;
```
Est: ~4-6x fewer system calls per file on typical text files. Significant for throughput on `cat large_file`.

### HIGH: getc() per byte in cook_buf() tight loop (cat.c:194)

```c
for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
```

Every byte of the file goes through `getc()` -- a libnix JSR + stack frame + buffer check. For a 10KB file with `-n` flag (line numbering), that is ~10000 getc() calls. On 7MHz 68000, each costs ~40-60 cycles overhead = 400-600K cycles just for input.

**Fix:** Replace with `fgets()` into a static line buffer, then process the buffer with pointer arithmetic:
```c
static char linebuf[512];
while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
    /* process linebuf char-by-char using a pointer, no JSR overhead per char */
}
```
This reduces function call overhead by ~80x for average 80-char lines. The inner pointer scan is pure register operations on 68000 (no JSR).

Note: `cook_buf` handles -n (line numbering), -s (squeeze blank lines), -e/$-marking, -t/^I-marking, and -v (visible). The per-character logic is complex but can still be processed after reading a line.

**Constraint:** Lines longer than the buffer require either a larger buffer or a fallback to the getc path. A 512-byte static buffer covers >99% of real text. The static keyword avoids stack use (safe: cook_buf is not recursive).

Est: 3-5x throughput improvement for cat with formatting flags on typical text files.

### LOW: fprintf() for line numbers in cook_buf() (cat.c:207)

```c
fprintf(stdout, "%6lu\t", ++line);
```

One fprintf() per line when -n is active. For a 1000-line file this is 1000 fprintf() calls. Could be replaced with manual number formatting + fwrite, but this is LOW impact since it's one call per line vs. one getc() per character.

### NO ISSUES: raw_cat() read/write loop structure

The write-retry loop `for (off = 0; nr; nr -= nw, off += nw)` is correctly structured to handle short writes. No issues.

### NO ISSUES: Stack size

`__stack = 8192` is adequate. The `static char *buf` in raw_cat() is heap-allocated, not on stack. No large local arrays.

## Summary

- **Primary bottleneck:** For default `cat file`: disk I/O (BUFSIZ=512 means excessive Read() calls). For `cat -n file`: getc() per-byte call overhead.
- **Estimated overall impact:** Significant for the raw_cat BUFSIZ fix (4-6x fewer syscalls). Significant for the cook_buf fgets rewrite (3-5x throughput).
- **Both HIGH findings affect common usage patterns** (`cat file` and `cat -n file` are both very common).
