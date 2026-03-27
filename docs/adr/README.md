# Architecture Decision Records (ADR)

This directory contains Architecture Decision Records for the amiport project.

ADRs capture significant technical and architectural decisions with their context, rationale, and consequences. They are immutable once accepted — superseded decisions are marked as such with a link to the replacement.

## Index

| ID | Title | Status | Date |
|----|-------|--------|------|
| [ADR-001](001-posix-shim-over-inline-rewrites.md) | POSIX shim library over inline rewrites | Accepted | 2026-03-19 |
| [ADR-002](002-docker-first-toolchain.md) | Docker-first cross-compilation toolchain | Accepted | 2026-03-19 |
| [ADR-003](003-amigaos-3x-primary-target.md) | AmigaOS 3.x as primary target | Accepted | 2026-03-19 |
| [ADR-004](004-vamos-for-testing.md) | vamos for CLI testing over full emulation | Accepted | 2026-03-19 |
| [ADR-005](005-prebuilt-docker-images.md) | Prebuilt Docker images for toolchain | Accepted | 2026-03-19 |
| [ADR-006](006-m68000-target-for-vamos.md) | m68000 target for vamos compatibility | Accepted | 2026-03-19 |
| [ADR-007](007-toolchain-wrapper-scripts.md) | Toolchain wrapper scripts | Accepted | 2026-03-19 |
| [ADR-008](008-tiered-posix-compatibility.md) | Tiered POSIX compatibility strategy | Accepted | 2026-03-20 |
| [ADR-009](009-console-shim-for-ncurses.md) | Console UI shim for ncurses porting | Accepted | 2026-03-20 |
| [ADR-010](010-bsdsocket-shim-for-networking.md) | BSD socket shim for network app porting | Accepted | 2026-03-20 |
| [ADR-011](011-beyond-cli-port-categories.md) | Beyond CLI — expanding port target categories | Accepted | 2026-03-20 |
| [ADR-012](012-post-porting-quality-gates.md) | Post-porting quality gates | Accepted | 2026-03-20 |
| [ADR-013](013-port-directory-hygiene.md) | Port directory hygiene | Accepted | 2026-03-20 |
| [ADR-014](014-fs-uae-automated-testing.md) | FS-UAE automated testing pipeline | Accepted | 2026-03-21 |
| [ADR-015](015-toolkit-quality-infrastructure.md) | Toolkit quality infrastructure (CI, memory-checker split, hooks, check-docs) | Accepted | 2026-03-21 |
| [ADR-016](016-autonomous-debug-agent.md) | Autonomous debug agent with Enforcer integration | Accepted | 2026-03-21 |
| [ADR-017](017-pretooluse-hooks-for-structural-enforcement.md) | PreToolUse hooks for structural enforcement | Accepted | 2026-03-21 |
| [ADR-018](018-adcd-reference-knowledge-base.md) | ADCD reference knowledge base as agent context | Accepted | 2026-03-21 |
| [ADR-019](019-agent-persona-and-tool-matrix.md) | Agent persona and tool matrix | Accepted | 2026-03-21 |
| [ADR-020](020-git-hooks-for-doc-and-hygiene-validation.md) | Git hooks for documentation and hygiene validation | Accepted | 2026-03-21 |
| [ADR-021](021-design-system-mui-warm-gray.md) | Website design system — Amiga MUI on warm gray | Accepted | 2026-03-22 |
| [ADR-022](022-c99-compiler-support.md) | C99 compiler support for complex ports | Accepted | 2026-03-22 |
| [ADR-023](023-automated-interactive-testing.md) | Automated interactive testing via KeyInject | Accepted | 2026-03-22 |
| [ADR-024](024-visual-verification.md) | FS-UAE visual verification via ANSI capture | Accepted | 2026-03-24 |
| [ADR-025](025-screen-read-trap.md) | FS-UAE custom trap for interactive visual verification | Accepted | 2026-03-24 |
| [ADR-026](026-cpython-port.md) | CPython 3.11.12 port architecture | Accepted | 2026-03-27 |

## Template

Use [TEMPLATE.md](TEMPLATE.md) when creating new ADRs.
