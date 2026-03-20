---
name: port-project
description: Port an entire C project to AmigaOS. Runs the full pipeline — analyze, transform, build, test, package. Use to port a new project from scratch.
argument-hint: <path-to-source>
allowed-tools: Read, Write, Edit, Bash, Grep, Glob, Agent
---

# Port Project to Amiga

You are the orchestrator for porting a C project to AmigaOS 3.x. You run the full pipeline end-to-end.

## Pipeline

### Stage 1: Analyze
Dispatch a `source-analyzer` agent to analyze the source, or run the analysis inline for small projects (< 5 files).

Review the portability report. If the verdict is **INFEASIBLE**, stop and explain why. For other verdicts, proceed.

### Stage 2: Transform
Dispatch a `code-transformer` agent, or for small projects, apply transformations inline following the rules in `.claude/skills/transform-source/references/transformation-rules.md`.

This creates a `ported/` directory with the transformed source.

Before transforming, verify which `amiport_*` shim functions actually exist by checking headers in `lib/posix-shim/include/amiport/`. If a needed wrapper is missing, either add it to the shim or stub the call.

### Stage 3: Build
Dispatch a `build-manager` agent to cross-compile.

If the build fails, iterate: read the errors, fix the transformed source, and rebuild. Maximum 5 iterations.

### Stage 4: Test
Dispatch a `test-runner` agent to verify the binary works correctly in vamos.

If tests fail, analyze the failure and fix. May require going back to the transform stage.

### Stage 5: Package
Create an LHA archive with the binary, readme, and any documentation.

## Orchestration

For larger projects, use the Agent tool to dispatch specialized agents in parallel where possible:
- `source-analyzer` — for analysis (can run independently)
- `code-transformer` — for transformation (depends on analysis)
- `build-manager` — for compilation (depends on transformation)
- `test-runner` — for testing (depends on build)

For small projects (1-3 files), running everything inline is faster than agent dispatch overhead.

## Error Recovery Between Stages

When a stage fails, don't just retry — diagnose which stage needs to be revisited:

| Symptom | Root Cause | Go Back To |
|---------|-----------|------------|
| Missing header during build | Incomplete transformation | Transform |
| Undefined `amiport_*` function | Shim doesn't have this wrapper | Add to `lib/posix-shim/` |
| Type mismatch error | Wrong POSIX→Amiga type mapping | Transform |
| Linker error: undefined symbol | Function not stubbed or shimmed | Transform or extend shim |
| Program crashes in vamos | Logic error in transformation | Debug output, fix transform |
| Wrong output in vamos | Behavioral difference in shim | Fix shim implementation |
| vamos "unknown library" | Program uses unsupported Amiga lib | Stub the library call |

## Porting Log

Maintain a `PORT.md` file in the project directory documenting:
- Original project name, version, and source
- Portability analysis summary
- List of all transformations applied
- Build configuration (compiler, target, flags)
- Test results
- Known limitations

## Decision Points

At each stage, you may need to make judgment calls:

1. **Blocking features**: Stub them or skip them? Default: stub with a warning message if the feature is non-essential. If it's core functionality, stop and ask.

2. **Multiple source files**: Transform all files, build all objects, link together. Handle Makefile translation.

3. **External dependencies**: If the project depends on external libraries, check if they've been ported. If not, note this as a limitation.

4. **AmigaOS version**: Default to 3.x. Only target earlier versions if specifically requested.

## Boilerplate

Every ported program gets a version string with today's date:
```c
static const char *verstag = "$VER: progname 1.0 (DD.MM.YYYY)";
```
Use the actual current date in DD.MM.YYYY format.

## Success Criteria

A port is successful when:
- The binary compiles without errors
- Basic functionality works in vamos
- Output matches the native version for standard test cases
- PORT.md documents the entire process
