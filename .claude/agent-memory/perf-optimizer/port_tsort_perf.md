---
name: port_tsort_perf
description: Performance findings for ports/tsort 1.38 — ohash hash function multiply-heavy (MEDIUM), strlen after fgets (LOW), read_pairs architecture good reviewed 2026-03-26
type: project
---

## tsort 1.38 — Performance Analysis

**Primary bottleneck:** CPU-bound during input parsing; graph traversal for large inputs.

### Hot Paths
- read_pairs(): fgets loop + isspace scan + ohash_qlookupi per word
- ohash_hash_interval(): h * 31 + c per byte — called for every node name on every lookup
- insert_arc(): O(degree) linear scan for duplicate check

### HIGH Findings
None. The architecture is sound for typical tsort input sizes (makefile deps = hundreds of nodes).

### MEDIUM Findings
1. ohash_hash_interval() uses `h = h * 31 + (unsigned char)*p++` (tsort.c line ~99-103).
   On 68000, MULU takes 38-70 cycles per multiply. For a 10-char node name = 10 MULU ops per
   hash. On 68020+ (A1200) MULU drops to 20-28 cycles. The `* 31` can be replaced with
   `(h << 5) - h` — a shift+subtract, costing ~8 cycles total vs 38-70.
   ```c
   h = (h << 5) - h + (unsigned char)*p++;
   ```
   Est: 5-10% speedup on input-heavy workloads (many unique node names) on 68000. Negligible
   on 68020+ where MULU is pipelined.

2. strlen(line_buf) called after fgets in read_pairs() and read_hints() (lines ~569, ~629).
   This rescans the buffer that fgets just wrote. fgets returns the number of chars written
   only via NULL/non-NULL, not length. Workaround: scan for '\n' or '\0' to find end,
   or accept the strlen cost since it's O(line) and done once per line. Low priority.

3. read_pairs() uses `size = strlen(line_buf)` then manually scans for words. Since the buffer
   is already in registers/cache after fgets, the strlen penalty is small. Accept as-is.

### LOW
4. insert_arc() walks the entire arc list to check for duplicates (tsort.c line ~533-536).
   For graphs where one node has many outgoing edges, this is O(degree^2). Typical makefile
   deps don't hit this. For worst-case inputs with highly-connected nodes, a hash set per
   node would help, but this is out of scope for normal use.

5. READ_PAIRS_BUF and READ_HINTS_BUF are both 4096 bytes as static arrays inside the
   functions. On AmigaOS with static local arrays this is fine (single-threaded, non-recursive).
   The static keyword is already present. Stack safety confirmed.

### Stack / Safety
- __stack = 32768 (tsort uses 32KB). read_pairs has static char line_buf[4096] — correctly
  static. read_hints has static char line_buf[4096] — correctly static. Both fine.
- ohash_insert() can recurse into ohash_qlookupi during resize — but this is not real
  recursion (no stack frames piling up), just function calls. Stack safe.
- Graph traversal algorithms (find_cycle_from, traverse_node) are iterative with explicit
  stacks (struct array). No deep C recursion. Safe.
