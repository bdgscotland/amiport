---
name: audit_68k_hardware_md
description: Audit of docs/references/68k-hardware.md — 6 issues found, 4 auto-fixed on 2026-03-21
type: project
---

Audit of `/Users/duncan/Developer/amiport/docs/references/68k-hardware.md` run 2026-03-21.

**Fixed (auto-applied):**

1. ISSUE 1 (error): 0x200000–0x9FFFFF was labeled "Ranger/slow RAM or Zorro II expansion memory". Ranger/slow RAM is 0xC00000–0xD7FFFF. Fixed to "Zorro II expansion + motherboard Fast RAM".

2. ISSUE 2 (warning): Kickstart ROM size was tied to machine model (A500/A2000 = 256KB, A1200+ = 512KB). The determining factor is Kickstart version. Fixed to "256KB on KS 1.x/2.x mirrored at $FC0000; 512KB on KS 3.x fills full range".

3. ISSUE 3 (warning): Unverified Zorro III I/O row (0x10000000–0x3FFFFFFF) removed. Verified Zorro III layout: memory at $40000000–$7FFFFFFF, autoconfig at $FF000000.

4. ISSUE 4 (note): "68000 / 68010" in CPU table removed — 68010 was never in any production Amiga. Fixed to 68000 only, with parenthetical note.

**Flagged but not auto-fixed (prose judgment calls):**

5. ISSUE 5 (note): Function prologue example includes A6 in MOVEM.L save list. GCC-generated AmigaOS code typically does not save A6 in prologues — it is reloaded before each library call. Could mislead debug-agent.

6. ISSUE 6 (note): Guru code decomposition "0xATDDDDEE" notation is imprecise — under the notation, 0x81000009 decodes as subsystem 0x0000, but the table says it is exec (subsystem 0x0001). Code values are correct; only the structural decomposition is misleading.

**Why:** This file is used by debug-agent, code-transformer, and build-manager for hardware fact lookup. Incorrect claims propagate to all agents.
**How to apply:** When auditing other docs files or agent definitions that cite hardware facts, cross-check against the 6 knowledge domains in the hardware-expert system prompt, not this file alone.
