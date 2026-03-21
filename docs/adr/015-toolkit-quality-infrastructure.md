# ADR-015: Toolkit Quality Infrastructure

## Status
Accepted (amends ADR-012)

## Date
2026-03-21

## Context

After shipping 5 ports (cal, cut, diff, grep, sed) and starting Lua, several recurring friction points emerged in the porting toolkit itself:

1. **Memory safety conflated with performance tuning.** The `perf-optimizer` agent handled both mandatory memory leak detection and optional 68k performance optimization. ADR-012 described it as mandatory, but the performance tuning was optional — confusing for dispatching agents.

2. **No CI.** Regressions in the shim library were invisible until the next port hit them. No automated validation that examples still build and tests still pass.

3. **Documentation drift.** The project requires updating 7+ files for any agent/skill change. ADR-013 was missing from the ADR index, proving manual sync fails.

4. **No structural enforcement.** Rules told agents not to edit `original/` or call compilers directly, but nothing enforced it. Behavioral rules depend on agent compliance.

5. **Missing shim functions.** `dup()/dup2()` were needed for I/O redirection (blocking the Lua port) but not yet implemented.

## Decision

### 1. Split perf-optimizer into two agents (amends ADR-012)

- **`memory-checker`** (Stage 6b, **mandatory**): Finds memory leaks, double-frees, unsafe realloc, file handle leaks. Must run on every port.
- **`perf-optimizer`** (Stage 6c, **optional**): 68k instruction timing, loop optimization, memory access patterns. Run for performance-critical ports.

Pipeline stages become: 6a=review-amiga, 6b=memory-checker, 6c=perf-optimizer, 6d=knowledge-capture.

### 2. Add GitHub Actions CI with GHCR caching

- CI runs on every push: build shim/emu, run tests, check-docs, build examples
- Toolchain Docker image cached on GHCR (free for public repos)
- Self-bootstrapping: first run builds image, subsequent runs pull cache

### 3. Add `make check-docs` for automated doc consistency

- Extracts agent names from `.claude/agents/*.md`
- Greps for each name in CLAUDE.md, README.md, docs/architecture.md
- Name-set matching (not just count) — reports which agents are missing where
- Strict match, no exemptions

### 4. Add native PreToolUse hooks for structural safety

- **`block-original-edits.sh`**: Blocks Edit/Write to paths containing `/original/`
- **`block-direct-gcc.sh`**: Blocks direct `m68k-amigaos-gcc`/`ld`/`as` calls, forces use of make/wrapper scripts
- These are structural enforcement (hook exits non-zero) rather than behavioral rules (prose instructions)

### 5. Add knowledge capture step (Stage 6d)

- After each port, review PORT.md for novel transformation patterns and pitfalls
- Feed discoveries back into `transformation-rules.md`, `known-pitfalls.md`, `posix-tiers.md`
- Ensures lessons learned compound across future ports

### 6. Implement dup()/dup2() in posix-shim

- Parallel `fd_closeable[]` array tracks whether `Close()` should be called
- On close, scan fd_table for shared BPTRs before calling `Close()`
- fds 0-2 (stdin/stdout/stderr) marked non-closeable by default
- `dup2(file, 1)` makes fd 1 closeable (correct for stdout redirection)

### 7. Add agent memory and worktree isolation

- `memory: project` on source-analyzer, code-transformer, build-manager for accumulated insights
- `isolation: worktree` on port-coordinator for structural file isolation
- Path scoping on amiga-coding.md and known-pitfalls.md rules (only load for C files)

## Consequences

### Positive
- Memory safety is now a clear, mandatory gate separate from optional performance tuning
- CI catches regressions automatically on every push
- Documentation consistency is machine-verified, not human-verified
- Structural hooks enforce safety regardless of agent behavior
- dup/dup2 unblocks the Lua port and future I/O redirection ports
- Knowledge compounds across ports via the feedback loop

### Negative
- 10 agents (up from 9) — more to maintain, but check-docs catches drift
- CI adds ~5-minute latency to pushes (acceptable for regression prevention)
- Native hooks add complexity to settings.json (but replace fragile behavioral rules)

### Neutral
- perf-optimizer agent still exists but with narrowed scope — no content was lost, just separated
