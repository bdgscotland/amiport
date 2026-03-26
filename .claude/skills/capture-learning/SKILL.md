---
name: capture-learning
description: Capture a lesson learned from a porting session and route it to the right enforcement mechanism. Use when a bug, mistake, or process failure occurs that should be prevented in future sessions.
---

# Capture Learning

You just discovered something that should prevent future mistakes. Route it to the right place.

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

From strongest to weakest:

1. **Hook** (settings.json) — Fires on every tool call. Cannot be bypassed. Use for absolute rules.
2. **Rule** (.claude/rules/) — Loaded into every conversation. Agents see it. Can be ignored by careless agents.
3. **Agent instruction** (.claude/agents/) — Only affects that specific agent. Good for agent-specific behavior.
4. **Skill instruction** (.claude/skills/) — Only loaded when skill is invoked. Good for workflow steps.
5. **Known pitfall** (.claude/rules/known-pitfalls.md) — Reference material. Agents must actively check it.
6. **Memory** (~/.claude/projects/.../memory/) — Recalled on relevance. Weakest — no enforcement.

## Template

When invoking this skill, state:

1. **What happened** (one sentence)
2. **What should have happened** (one sentence)
3. **Where to enforce it** (pick from decision tree)

Then make the change immediately — don't ask for permission.

## After Capturing

Verify the learning is discoverable:
- If it's a rule: will agents see it? Check the `Paths:` filter at top of the rule file.
- If it's a pitfall: is it in known-pitfalls.md AND crash-patterns.md (if applicable)?
- If it's an agent change: does the agent's skill injection include the updated reference?
- If it's a hook: does it fire on the right tool calls?
