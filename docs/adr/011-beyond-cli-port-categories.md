# ADR-011: Beyond CLI — Expanding Port Target Categories

## Status

Accepted

## Date

2026-03-20

## Context

amiport was built for porting POSIX CLI utilities (wc, head, diff, cal). The pipeline (analyze → transform → build → test) and the tiered shim model (ADR-008) work well for this class of software. But the Amiga community needs more than coreutils — they want interactive console programs (less, vim), network tools (curl, irc), and scripting interpreters (Lua, bc).

Each category has different porting challenges:

1. **CLI tools** (current scope) — Pure POSIX function mapping. Solved by posix-shim/emu.
2. **Scripting interpreters** — Mostly portable C, but may use dlopen, longjmp/setjmp, or platform-specific tricks. Usually achievable with the existing pipeline plus minor shim additions.
3. **Console UI apps** — Use ncurses/termcap for screen control. Requires a terminal abstraction layer mapping to Amiga console.device.
4. **Network apps** — Use BSD sockets. Requires bsdsocket.library integration (ADR-010).
5. **GUI apps** — Use platform-specific toolkits. Requires Intuition/GadTools/MUI knowledge. Fundamentally creative, not mechanical.

## Decision

Expand amiport to support five port categories, in this priority order:

### Category 1: CLI tools (existing)
No changes needed. The posix-shim/emu pipeline handles this.

### Category 2: Scripting interpreters
Use the existing pipeline. Add shims as needed (e.g., `dlopen` stubs, `setlocale` stubs). Port Lua as the proof-of-concept to validate that the existing infrastructure works for larger, non-trivial codebases.

### Category 3: Console UI apps
Add `lib/console-shim/` (ADR-009) providing a subset of ncurses mapped to Amiga console.device ANSI escape sequences. The pipeline gains awareness of console UI programs and recommends FS-UAE testing instead of vamos-only.

### Category 4: Network apps
Add `lib/bsdsocket-shim/` (ADR-010) providing thin wrappers that handle bsdsocket.library open/close lifecycle and the close()→CloseSocket(), select()→WaitSelect() mappings. The analyzer gains awareness of socket dependencies.

### Category 5: GUI apps (future — see CIDR-002, CIDR-005)
Not yet in scope for implementation. Requires a gui-designer agent and fundamentally different transformation approach. Keep exploring via CIDRs.

### Pipeline impact

The `/analyze-source` skill should classify the port target category as part of its report:

```json
{
  "category": "console-ui",
  "rationale": "Uses ncurses (initscr, endwin, mvaddch, getch)",
  "recommended_shims": ["console-shim"],
  "test_strategy": "fs-uae"
}
```

The `/port-project` orchestrator should adapt its strategy based on category.

## Consequences

### Positive

- Clear scope expansion path without losing focus on CLI tools
- Each category adds to the toolkit incrementally
- Scripting interpreters can be attempted immediately with existing infrastructure
- Console and network shims are self-contained libraries that don't affect existing ports

### Negative

- More shim libraries to maintain (console-shim, bsdsocket-shim)
- Console UI testing requires FS-UAE (heavier than vamos)
- Network testing requires a TCP/IP stack in the emulator environment
- Risk of scope creep — must resist jumping to GUI (Category 5) before Categories 2-4 are solid

### Neutral

- The tiered model (ADR-008) still applies: each new shim library is Tier 1 or Tier 2 depending on fidelity
- The existing agents (source-analyzer, code-transformer, build-manager) need minor updates to recognize new patterns, not wholesale replacement
