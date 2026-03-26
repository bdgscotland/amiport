---
name: port_column_ln_expr_look_tty_perf
description: Performance findings for ports/column, ln, expr, look, tty — batch review 2026-03-26
type: project
---

Reviewed 2026-03-26.

## column 1.27
- MEDIUM: putchar() per tab in c_columnate/r_columnate inner loops. Build tab string with fwrite.
- LOW: strchr(separator) called in field-scan inner loop. Cache separator[0] and use direct compare when single-char (common case).
- CLEAN: getline() usage is correct.

## ln 1.25
- LOW: Single-shot filesystem tool. No I/O loops, no hot paths. Clean bill of health.

## expr 1.28
- MEDIUM: strcoll() for string comparisons in eval2() — costly on 68000, use strcmp() when locale is irrelevant (AmigaOS has no locale).
- MEDIUM: int64_t (long long) arithmetic throughout. 64-bit ops on 68000 emit multi-word emulation sequences. Acceptable for eval() but worth documenting.
- MEDIUM: make_int()/make_str() allocate struct val via malloc() for every token evaluation. Hot path in scripting loops.
- LOW: Single-shot tool. Not hot in practice.

## look 1.27
- HIGH: putchar() per character in print_from() inner output loop. Should batch with fwrite().
- LOW: compare() function called in tight binary/linear search loops — but work is pointer arithmetic on in-memory data, acceptable.

## tty 1.14
- CLEAN: Trivial single-shot tool. IsInteractive() call, one puts(). No optimization opportunities.
