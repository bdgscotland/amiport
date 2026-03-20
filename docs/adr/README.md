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

## Template

Use [TEMPLATE.md](TEMPLATE.md) when creating new ADRs.
