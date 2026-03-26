---
name: extend-shim
description: Add a new POSIX function to the amiport shim (Tier 1) or emulation (Tier 2) library. Researches AmigaOS APIs first, then implements with proper mapping. Looks up the POSIX spec, classifies the tier, writes the implementation + header + test, and rebuilds the library.
argument-hint: <function-name>
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, Agent, WebSearch, WebFetch, Skill
---

# Extend Shim Skill

You are adding the POSIX function `$ARGUMENTS` to the amiport compatibility libraries. Follow these steps precisely.

## Input

The function to add: `$ARGUMENTS`

## Step 1: Load Required Skills

Before doing ANY research or writing ANY code, load these skills:

1. **`/c89-reference`** — all shim code must be ANSI C89. Load the constraint set.
2. **`/crash-patterns`** — check if this function has known pitfalls from prior ports.
3. **`/amiga-api-lookup`** — load the ADCD reference library. This is NOT optional.

## Step 2: Research — POSIX Side

1. **Check if it already exists** — search `lib/posix-shim/include/amiport/` and `lib/posix-emu/include/amiport-emu/` for the function name
2. **Look up the POSIX spec** — understand the function signature, return value, error codes, and edge cases
3. **Check the tier classification** in `docs/posix-tiers.md`:
   - If already classified, note it (but do NOT skip Step 3 — the tier may be wrong if AmigaOS research wasn't done originally)
   - If not classified, you will determine the tier after Step 3

## Step 3: Research — AmigaOS Side (MANDATORY GATE)

**This step is a hard gate. Do NOT proceed to classification or implementation without completing it.**

1. **Search the ADCD function index** — `docs/references/adcd/FUNCTIONS.md` for AmigaOS equivalents. Search by concept, not just name (e.g., for `setlocale`, search "locale"; for `getpwnam`, search "user" or "owner").
2. **Read the autodoc documentation** — for every candidate AmigaOS function, read the full autodoc (signature, inputs, result, notes, caveats). These are in `docs/references/adcd/autodocs-3.5/`.
3. **Read the prose/guide documentation** — check `docs/references/adcd/libraries/` for broader context on the relevant library.
4. **Read the header file** — check `docs/references/adcd/autodocs-3.5/include-*.md` for struct layouts and constants.
5. **Read code examples** — check `docs/references/adcd/examples/` for canonical Commodore usage patterns.
6. **Check known pitfalls** — does `docs/references/crash-patterns.md` mention this function or the AmigaOS API you plan to use?

**If the AmigaOS API involves hardware, memory layout, struct offsets, or device I/O:**

7. **Dispatch `hardware-expert` agent** to validate your assumptions:
   ```
   Agent(subagent_type="hardware-expert", prompt="I'm implementing a POSIX shim for [function]. I plan to use AmigaOS [API]. Please validate: [specific hardware/memory/struct assumptions].")
   ```

**Decision point after this step:**
- If AmigaOS has a direct or near-direct API → this is likely **Tier 1** (real mapping, not a stub)
- If AmigaOS has a partial API or no equivalent → this is **Tier 2** (emulation with documented caveats)
- If the concept cannot be meaningfully implemented → this is **Tier 3** (redesign pattern, no shim)

## Step 4: Classify

Determine the tier based on your research from Steps 2 AND 3:

- **Tier 1 (Shim)**: Direct or near-direct mapping to an AmigaOS API. Goes in `lib/posix-shim/`.
  - Function prefix: `amiport_`
  - Header location: `lib/posix-shim/include/amiport/`
  - Source location: `lib/posix-shim/src/`
- **Tier 2 (Emulation)**: Approximate mapping with documented caveats. Goes in `lib/posix-emu/`.
  - Function prefix: `amiport_emu_`
  - Header location: `lib/posix-emu/include/amiport-emu/`
  - Source location: `lib/posix-emu/src/`
- **Tier 3 (Redesign)**: Cannot be shimmed. Do NOT create a shim — instead, add a redesign pattern to `.claude/skills/transform-source/references/redesign-patterns.md` and update `docs/posix-tiers.md`.

**Important:** A function previously classified as Tier 2 (stub) may be reclassified as Tier 1 if AmigaOS research reveals a real API. The ADCD documentation is authoritative — prior tier classifications made without ADCD research may be wrong.

## Step 5: Implement

### For Tier 1 functions:

1. **Decide header placement** — add to an existing header if it fits (e.g., file I/O goes in `amiport/unistd.h`, directory ops in `amiport/dirent.h`) or create a new header if it's a new category
2. **Write the function declaration** in the header with a documentation comment explaining:
   - What the POSIX function does
   - Which AmigaOS API it maps to (cite the ADCD autodoc reference)
   - How the Amiga implementation differs (if at all)
3. **Write the implementation** in the appropriate `.c` file (or new file if new category)
   - Use ANSI C89 (verified against `/c89-reference` constraints loaded in Step 1)
   - Include proper AmigaOS headers (`<proto/*.h>`)
   - **Use the AmigaOS API discovered in Step 3** — do NOT write a stub if a real API exists
   - Use `amiport_map_errno()` for error mapping
   - Add to the Makefile SRCS list if creating a new `.c` file
   - Open/close AmigaOS libraries properly (OpenLibrary/CloseLibrary pattern)
4. **Add a convenience macro** for drop-in replacement, guarded by `AMIPORT_NO_xxx_MACROS`

### For Tier 2 functions:

1. **Create header** with `/* EMULATION NOTICE */` block listing:
   - What AmigaOS API was researched and why it doesn't fully cover the POSIX semantics
   - Exactly which behaviors are approximated vs missing
2. **Create source file** with the implementation
   - Use any relevant AmigaOS APIs for partial coverage (don't stub if you can partially implement)
3. **Add `AMIPORT_EMU_xxx_ENABLED` compile-time flag**
4. **Update `amiport-emu.h`** master header to include the new header

## Step 6: Write Tests

Create or extend a test file in `tests/shim/` (Tier 1) or `tests/emu/` (Tier 2):

1. Use the test framework from `tests/shim/test_framework.h`
2. Include the Amiga version string and stack cookie boilerplate
3. Test at minimum:
   - Basic happy path
   - Error cases (invalid arguments, file not found, etc.)
   - Edge cases specific to the function
   - Any Amiga-specific behaviour differences
   - Verify that the AmigaOS backend is actually called (not just returning hardcoded values)
4. Add the test to the appropriate Makefile's TESTS list

## Step 7: Update Documentation and Catalog

### Shim-internal docs (always):

1. **`docs/posix-tiers.md`** — add the function to the appropriate tier table. If reclassifying from a prior tier, note why (e.g., "Reclassified T2->T1: locale.library provides OpenLocale()")
2. **`.claude/skills/analyze-source/references/posix-to-amiga-map.md`** — add the mapping entry, including the AmigaOS API used
3. **`.claude/skills/transform-source/references/transformation-rules.md`** — add the transformation rule if applicable

### Catalog (always — this is how ports get unblocked):

4. **`data/catalog.json`** — update the `shim_coverage` section:
   - Add the new function to the available shims list
   - Remove it from `missing_posix` for any candidates that were blocked on it
   - Run `python3 scripts/catalog-score.py --score --status` to verify readiness scores changed
5. **Run `python3 scripts/catalog-score.py --status`** — confirm the "shim unlock opportunities" table reflects the change and report how many programs were unblocked

### Project-wide docs (when applicable):

6. **`CLAUDE.md`** — update if you added a new header file, new library category, or new shim pattern that agents need to know about
7. **`docs/architecture.md`** — update if you added a new source file or header category to `lib/posix-shim/` or `lib/posix-emu/`
8. **`README.md`** — update shim coverage stats if mentioned

**Do not consider the shim extension complete until the catalog is updated.** The whole point of extending the shim is to unblock ports — if the catalog doesn't know about it, ports stay blocked.

## Step 8: Build and Verify

```bash
make build-shim    # or make build-emu for Tier 2
make test-shim     # or run specific test via vamos
```

Fix any compilation errors. Iterate until clean.

## Step 9: Report

Summarize what was added:
- Function name and signature
- Tier classification (and reclassification rationale if changed)
- AmigaOS API used (cite the ADCD autodoc)
- Any caveats or limitations
- Files created/modified
