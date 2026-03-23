# Perf Optimizer Memory Index

- [port_tail_perf.md](port_tail_perf.md) - Performance findings for ports/tail — hotpath summary and recommendations
- [port_wc_perf.md](port_wc_perf.md) - Performance findings for ports/wc — hotpath summary and recommendations reviewed 2026-03-22
- [port_lua_perf.md](port_lua_perf.md) - Performance findings for ports/lua 5.4.7 — LUA_32BITS, localeconv override, match_class optimization reviewed 2026-03-22
- [port_uniq_perf.md](port_uniq_perf.md) - Performance findings for ports/uniq 1.33 — 64-bit type downgrade, isblank inline, function pointer hoist reviewed 2026-03-22
- [port_patch_perf.md](port_patch_perf.md) - Performance findings for ports/patch v1.78 — putc hotpath, pgetline correctness bug, I/O bound analysis reviewed 2026-03-23
- [port_jq_perf.md](port_jq_perf.md) - Performance findings for ports/jq 1.7.1 — put_char fwrite per character, MurmurHash3 unaligned reads, builtin startup cost reviewed 2026-03-23
- [port_bc_perf.md](port_bc_perf.md) - Performance findings for ports/bc 1.07.1 — DIVU in multiply loops, malloc pressure in execute stack, MUL_BASE_DIGITS tuning reviewed 2026-03-23
