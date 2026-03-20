# CIDR-001: Agent Teams for Complex Multi-File Ports

## Status

Exploring

## Date

2026-03-19

## Idea

For large projects (hundreds of source files), use Claude Code teams to parallelize the porting work. Multiple code-transformer agents work on different files simultaneously, coordinated by a port-coordinator that maintains consistency.

## Motivation

Single-agent porting is sequential and slow for large codebases. Real-world software like `less`, `vim`, or `lua` has dozens to hundreds of source files. Parallelizing transformation could make these ports feasible.

## Open Questions

- How do we handle cross-file dependencies (shared headers, global state)?
- What's the right granularity — per-file, per-module, per-subsystem?
- How does the coordinator merge potentially conflicting shim decisions?
- What's the context window impact of large projects?

## Early Thinking

The coordinator agent could first produce a dependency graph, then assign independent subtrees to transformer agents. Shared headers get transformed first as a "foundation pass". Each agent writes to a porting log that the coordinator reviews for consistency.

## Next Steps

- Get single-agent pipeline working end-to-end first (MVP)
- Try a 5-10 file project manually to understand coordination challenges
- Prototype team-based approach with a medium-sized utility (e.g., `less`)
