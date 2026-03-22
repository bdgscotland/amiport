---
name: extend-shim
description: Add a new POSIX function to the amiport shim (Tier 1) or emulation (Tier 2) library. Looks up the POSIX spec, classifies the tier, writes the implementation + header + test, and rebuilds the library.
argument-hint: <function-name>
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, Agent, WebSearch, WebFetch
---

# Extend Shim Skill

You are adding the POSIX function `$ARGUMENTS` to the amiport compatibility libraries. Follow these steps precisely.

## Input

The function to add: `$ARGUMENTS`

## Step 1: Research

1. **Check if it already exists** — search `lib/posix-shim/include/amiport/` and `lib/posix-emu/include/amiport-emu/` for the function name
2. **Look up the POSIX spec** — understand the function signature, return value, error codes, and edge cases
3. **Check the tier classification** in `docs/posix-tiers.md`:
   - If already classified, use that tier
   - If not classified, use the decision tree in `docs/posix-tiers.md` to determine the tier
4. **Look up the AmigaOS equivalent** — invoke `/amiga-api-lookup` to find the AmigaOS function that implements the POSIX behavior. Search `docs/references/adcd/FUNCTIONS.md` for the function name, then read the autodoc AND prose documentation. Do NOT guess at function signatures or struct layouts.
5. **Read code examples** — check `docs/references/adcd/examples/` for canonical Commodore usage patterns

## Step 2: Classify

Determine the tier:

- **Tier 1 (Shim)**: Direct or near-direct mapping to AmigaDOS. Goes in `lib/posix-shim/`.
  - Function prefix: `amiport_`
  - Header location: `lib/posix-shim/include/amiport/`
  - Source location: `lib/posix-shim/src/`
- **Tier 2 (Emulation)**: Approximate mapping with documented caveats. Goes in `lib/posix-emu/`.
  - Function prefix: `amiport_emu_`
  - Header location: `lib/posix-emu/include/amiport-emu/`
  - Source location: `lib/posix-emu/src/`
- **Tier 3 (Redesign)**: Cannot be shimmed. Do NOT create a shim — instead, add a redesign pattern to `.claude/skills/transform-source/references/redesign-patterns.md` and update `docs/posix-tiers.md`.

## Step 3: Implement

### For Tier 1 functions:

1. **Decide header placement** — add to an existing header if it fits (e.g., file I/O goes in `amiport/unistd.h`, directory ops in `amiport/dirent.h`) or create a new header if it's a new category
2. **Write the function declaration** in the header with a documentation comment explaining:
   - What the POSIX function does
   - How the Amiga implementation differs (if at all)
3. **Write the implementation** in the appropriate `.c` file (or new file if new category)
   - Use ANSI C89
   - Include proper AmigaOS headers (`<proto/*.h>`)
   - Use `amiport_map_errno()` for error mapping
   - Add to the Makefile SRCS list if creating a new `.c` file
4. **Add a convenience macro** for drop-in replacement, guarded by `AMIPORT_NO_xxx_MACROS`

### For Tier 2 functions:

1. **Create header** with `/* EMULATION NOTICE */` block listing all behavioural differences
2. **Create source file** with the implementation
3. **Add `AMIPORT_EMU_xxx_ENABLED` compile-time flag**
4. **Update `amiport-emu.h`** master header to include the new header

## Step 4: Write Tests

Create or extend a test file in `tests/shim/` (Tier 1) or `tests/emu/` (Tier 2):

1. Use the test framework from `tests/shim/test_framework.h`
2. Include the Amiga version string and stack cookie boilerplate
3. Test at minimum:
   - Basic happy path
   - Error cases (invalid arguments, file not found, etc.)
   - Edge cases specific to the function
   - Any Amiga-specific behaviour differences
4. Add the test to the appropriate Makefile's TESTS list

## Step 5: Update Documentation

1. **`docs/posix-tiers.md`** — add the function to the appropriate tier table
2. **`.claude/skills/analyze-source/references/posix-to-amiga-map.md`** — add the mapping entry
3. **`.claude/skills/transform-source/references/transformation-rules.md`** — add the transformation rule if applicable

## Step 6: Build and Verify

```bash
make build-shim    # or make build-emu for Tier 2
make test-shim     # or run specific test via vamos
```

Fix any compilation errors. Iterate until clean.

## Step 7: Report

Summarize what was added:
- Function name and signature
- Tier classification
- AmigaOS implementation strategy
- Any caveats or limitations
- Files created/modified
