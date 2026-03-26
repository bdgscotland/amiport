---
name: port_test_perf
description: Performance findings for ports/test 1.23 — linear ops[] table scan, no I/O, single-shot expression evaluator reviewed 2026-03-26
type: project
---

# Performance Findings: ports/test 1.23

**Reviewed:** 2026-03-26

## Hot Paths

1. **t_lex() (test.c:698-715)** — Linear scan of ops[] table with strcmp() per entry. Called multiple times per expression evaluation.
2. **t_lex_type() (test.c:560-574)** — Same linear scan pattern. Called at least once per binary operation check.

## Key Findings

### LOW: Linear ops[] table scan in t_lex() and t_lex_type() (test.c:568, 706)

```c
struct t_op const *op = ops;
while (op->op_text) {
    if (strcmp(s, op->op_text) == 0)
        return op->op_num;
    op++;
}
```

The ops[] table has 38 entries. Each `t_lex()` call does up to 38 strcmp() comparisons. However, `test` is called once per shell conditional — total expression depth is typically 1-5 tokens. Even with 38 entries, the table fits in ~600 bytes and is sequential in memory. On 68000 this is ~500-1000 cycles worst case, which is negligible for a once-per-invocation cost.

A sorted table + bsearch would be O(log N) but adds complexity for ~5 extra strcmp calls at most. Not worth the change.

**Verdict:** No action needed. This is startup-time work.

### NO ISSUES: No file I/O loops

`test` reads no files. All operations are on argv strings and stat() calls on filenames.

### NO ISSUES: full_stat_get() wrapper (test.c:150-165)

The `full_stat_get()` wrapper calls `amiport_stat()` and copies fields. Two full_stat allocations per `-nt`/`-ot` operation (stack allocated, 28 bytes each). On 68000 struct copies at 28 bytes = 7 MOVE.L instructions -- negligible.

### NO ISSUES: Stack size

`__stack = 8192` is adequate. No large local buffers. Recursion depth for expression parsing is bounded by expression complexity (typically 3-5 levels for shell conditionals).

### NO ISSUES: getnstr() isspace/isdigit loops

The integer comparison path calls `getnstr()` which walks the string with `isspace()` and `isdigit()`. For typical integer strings of 1-10 chars, this is negligible.

## Summary

- **Primary bottleneck:** `amiport_stat()` calls for file tests (-f, -d, etc.) -- AmigaDOS Lock()/Examine() is inherently slow (50-200ms per call). This is unavoidable.
- **Estimated overall impact:** Marginal -- `test` is a single-shot utility with no data processing loops. CPU time is dominated by AmigaDOS filesystem calls.
- **No performance issues worth addressing found.**
