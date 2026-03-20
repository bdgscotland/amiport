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
Run `/analyze-source` on the provided source path.

Review the portability report. If the verdict is **INFEASIBLE**, stop and explain why. For other verdicts, proceed.

### Stage 2: Transform
Run `/transform-source` on the source.

This creates a `ported/` directory with the transformed source.

### Stage 3: Build
Run `/build-amiga` to cross-compile.

If the build fails, iterate: read the errors, fix the transformed source, and rebuild. Maximum 5 iterations.

### Stage 4: Test
Run `/test-amiga` to verify the binary works correctly in vamos.

If tests fail, analyze the failure and fix. May require going back to the transform stage.

### Stage 5: Package
Create an LHA archive with the binary, readme, and any documentation.

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

## Success Criteria

A port is successful when:
- The binary compiles without errors
- Basic functionality works in vamos
- Output matches the native version for standard test cases
- PORT.md documents the entire process
