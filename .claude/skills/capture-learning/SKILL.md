---
name: capture-learning
description: Capture a lesson learned from a porting session and route it to the right enforcement mechanism. Use when a bug, mistake, or process failure occurs that should be prevented in future sessions.
---

# Capture Learning

You just discovered something that should prevent future mistakes. Route it to the right place — **and to amiga-kb if it's universal knowledge.**

## Two Destinations, Not One

Every learning gets routed to **local enforcement** (hook / rule / agent / skill / memory — see Decision Tree below). Some learnings ALSO get routed to **amiga-kb MCP** so the knowledge is available across every amiport session and any future Amiga project that uses the shared KB.

### Does it belong in amiga-kb?

amiga-kb stores **universal Amiga/68k knowledge** — facts that are true about AmigaOS, Motorola 68k, bebbo-gcc, libnix, vamos, FS-UAE, and the broader porting ecosystem regardless of which project is porting what. If another agent, on another project, in another month, porting a different tool, could benefit from knowing this — it goes in amiga-kb.

**YES, route to amiga-kb when the learning is about:**
- AmigaOS behavior (dos.library, exec.library, intuition, graphics, etc.)
- 68k CPU quirks (alignment, endianness, instruction timing, FPU)
- bebbo-gcc / VBCC codegen bugs or extensions
- libnix function availability, behavior, or missing POSIX compliance
- Hardware constraints (chip RAM, stack, MMU, chipset differences)
- Crash signatures / Guru codes / Enforcer hits
- vamos emulation gaps (lock ceiling, math library, 68000 default)
- FS-UAE emulation gaps or config pitfalls
- Cross-cutting pitfalls (e.g. "ARexx is ASCII only", "AmigaDOS `:` is both volume and PATH separator")
- Recipes that would apply to ANY port (e.g. "how to suppress volume requesters", "how to get a console handle under Execute script")

**NO, do NOT route to amiga-kb when the learning is about:**
- amiport pipeline mechanics (agent discipline, skill workflow, hook config)
- amiport site architecture (htaccess, api/v1 proxy, PHP endpoints)
- Project-specific workflow (batch porting, catalog management, publishing)
- A specific port's local workaround (belongs in its PORT.md)
- User preferences or session context
- Repository conventions (commit format, directory layout, build targets)

**When in doubt, ask: "If I were porting Bochs on a fresh Linux box in 2028 with no amiport checkout, would this help?"** If yes, amiga-kb. If no, stay local.

### How to write to amiga-kb

Use these MCP tools (available via the amiga-kb MCP server — must be running via `docker compose up -d` in the amiga-kb repo):

- **`amiga_add_pitfall`** — Document a gotcha with title, body, severity, categories, source (e.g. `"project:amiport"`), and auto-edge extraction to linked APIs. Use for: "X behaves unexpectedly", "Don't do Y because Z", "libnix's A is actually B".
- **`amiga_add_crash_pattern`** — Document a crash with signature (Guru code, Enforcer output), root cause, fix, discovery context. Use for: reproducible crashes with identifying symptoms.
- **`amiga_ingest_doc`** — Add or update a reference document (disk + vectors + graph). Use for: longer-form material like a new autodoc or technique.
- **`amiga_report_gap`** — Report missing knowledge for future enrichment (lighter-weight than `add_pitfall`; use when you noticed something's missing but don't have the full fix yet).

**Always set `source_project: "amiport"`** on writes so cross-project analytics work. Always verify the server is reachable first (`amiga_health`) — if it's down, fall back to local-only and note a TODO to replay the write later.

### Dual-write workflow

When a learning is universal:

1. **First, write locally** following the Decision Tree below (hook/rule/agent/skill/memory). Local is the strongest enforcement for agents actively working in amiport.
2. **Then, write to amiga-kb** via `amiga_add_pitfall` or `amiga_add_crash_pattern`. This is the shared store — it doesn't replace local rules, it complements them so future project agents that don't have amiport checked out still get the knowledge.
3. **Cross-reference** — in the local rule/pitfall, mention the amiga-kb entry. In the amiga-kb entry body, mention where the local enforcement lives (`.claude/rules/known-pitfalls.md`, crash-patterns.md ID, etc.).

Never write ONLY to amiga-kb and skip local — amiga-kb is recall-on-relevance, local rules are loaded into every conversation. The KB complements local rules; it does not replace them.

## Decision Tree

Ask these questions in order:

### 1. Can it be enforced by a hook (deterministic, automated)?

If YES — write a PreToolUse or PostToolUse hook in `.claude/settings.json`.

Examples:
- "Never weaken test assertions" → hook that scans for EXPECT_CONTAINS: replacements in test files
- "Never edit original/ source" → block-original-edits.sh (already exists)
- "Never call gcc directly" → block-direct-gcc.sh (already exists)

Hooks are the strongest enforcement — they fire every time, no exceptions.

### 2. Is it a coding pattern that agents must follow?

If YES — add to the relevant rule file in `.claude/rules/`:

| Pattern | Rule File |
|---------|-----------|
| C coding on AmigaOS | `amiga-coding.md` |
| Test behavior | `mandatory-fsemu-testing.md` or `never-weaken-tests.md` |
| Port directory structure | `port-directory-hygiene.md` |
| Documentation updates | `documentation.md` |
| Known C/libnix pitfalls | `known-pitfalls.md` |

Rules are loaded into every conversation — agents see them automatically.

### 3. Is it a crash pattern or runtime bug?

If YES — add to `docs/references/crash-patterns.md` with:
- Unique ID (next sequential number)
- Signature (what you see when it happens)
- Root cause
- Fix
- Which port discovered it

### 4. Is it a transformation pattern the code-transformer should apply?

If YES — add to `.claude/skills/transform-source/references/transformation-rules.md`.

### 5. Is it an agent behavior that needs changing?

If YES — edit the agent definition in `.claude/agents/<agent-name>.md`:
- Add to its rules section
- Add validation steps to its checklist
- Update its skill injections if it needs new reference material

### 6. Is it a process/workflow improvement?

If YES — update the relevant skill:
- Port pipeline: `.claude/skills/port-project/SKILL.md`
- Test design: `.claude/agents/test-designer.md`
- Code review: `.claude/skills/review-amiga/SKILL.md`

### 7. Is it project context (not derivable from code)?

If YES — save to memory in `~/.claude/projects/-Users-duncan-Developer-amiport/memory/`.

## Enforcement Strength Hierarchy

From strongest to weakest (project-local only — amiga-kb is orthogonal and covered below):

1. **Hook** (settings.json) — Fires on every tool call. Cannot be bypassed. Use for absolute rules.
2. **Rule** (.claude/rules/) — Loaded into every conversation. Agents see it. Can be ignored by careless agents.
3. **Agent instruction** (.claude/agents/) — Only affects that specific agent. Good for agent-specific behavior.
4. **Skill instruction** (.claude/skills/) — Only loaded when skill is invoked. Good for workflow steps.
5. **Known pitfall** (.claude/rules/known-pitfalls.md) — Reference material. Agents must actively check it.
6. **Memory** (~/.claude/projects/.../memory/) — Recalled on relevance. Weakest — no enforcement.

**amiga-kb (MCP) is orthogonal, not a rank.** It's the shared-across-projects store for universal knowledge. An agent querying `amiga_search` / `amiga_pitfalls_for` / `amiga_api_lookup` / `amiga_crash_diagnosis` will find KB entries on demand. It doesn't compete with local rules — a universal pitfall belongs in BOTH `.claude/rules/known-pitfalls.md` (so every amiport session loads it) AND `amiga_add_pitfall` (so other projects and future sessions without the amiport checkout can still find it).

## Template

When invoking this skill, state:

1. **What happened** (one sentence)
2. **What should have happened** (one sentence)
3. **Universal or project-local?** (apply the YES/NO lists above)
4. **Where to enforce it locally** (pick from decision tree)
5. **If universal, which amiga-kb tool** (`amiga_add_pitfall` / `amiga_add_crash_pattern` / `amiga_ingest_doc` / `amiga_report_gap`)

Then make the changes immediately — don't ask for permission. Do local first, then amiga-kb if applicable.

## After Capturing

Verify the learning is discoverable:

**Local:**
- If it's a rule: will agents see it? Check the `Paths:` filter at top of the rule file.
- If it's a pitfall: is it in known-pitfalls.md AND crash-patterns.md (if applicable)?
- If it's an agent change: does the agent's skill injection include the updated reference?
- If it's a hook: does it fire on the right tool calls?

**amiga-kb (if universal):**
- Did the write succeed? Check the MCP tool's return value — don't assume.
- Is it searchable? Query `amiga_search` with a phrase from the entry; if it doesn't come back in the top results, the entry needs better tags or a more distinctive title.
- Does the local rule/pitfall cross-reference the amiga-kb entry, and vice versa?
- If `amiga_health` is down at capture time, note a TODO in the session and replay the write once the server is reachable — do not silently drop the universal learning.
