---
name: amiga-api-lookup
description: Look up AmigaOS API documentation from the ADCD reference library. Use when writing or reviewing code that calls AmigaOS functions (exec.library, dos.library, timer.device, intuition, graphics, etc.), designing shim functions, or needing to understand struct layouts, function signatures, or usage patterns. Also use when designing new AmigaOS-native features like profilers, crash handlers, or device drivers.
allowed-tools: Read, Grep, Glob
---

# AmigaOS API Lookup

You have access to the complete Amiga Developer CD v2.1 (ADCD) documentation, converted to markdown. This is the authoritative reference for ALL AmigaOS programming.

## How to Use This Skill

When you need AmigaOS API information, follow this lookup sequence:

### Step 1: Find the Function/Type

Search the master index first:

```
Grep pattern="FunctionName" path="docs/references/adcd/FUNCTIONS.md"
```

This returns links to both autodocs-2.0 and autodocs-3.5 versions. **Always prefer the 3.5 version** (it has more complete documentation).

For type/struct lookups:

```
Grep pattern="StructName" path="docs/references/adcd/TYPES.md"
```

### Step 2: Read the Autodoc

The autodoc gives you the function signature, inputs, outputs, notes, and bugs:

```
Read file_path="docs/references/adcd/autodocs-3.5/<library>-<function>-2.md"
```

Naming convention: `<library-name>-<function-name>-2.md` (the `-2` suffix means autodocs-3.5 version).

### Step 3: Read the Prose Documentation

The autodoc gives signatures; the prose docs give context, examples, and best practices:

```
Read file_path="docs/references/adcd/libraries/<chapter>.md"   # For libraries
Read file_path="docs/references/adcd/devices/<chapter>.md"      # For devices
```

### Step 4: Read Code Examples

Canonical Commodore code examples:

```
Glob pattern="docs/references/adcd/examples/**/*<keyword>*.c"
```

### Step 5: Check Amiga Mail

Developer tips and errata from Commodore's engineering team:

```
Grep pattern="keyword" path="docs/references/adcd/amiga-mail" output_mode="files_with_matches"
```

## Reference Map

| API Area | Autodocs | Prose | Examples | Key Functions |
|----------|----------|-------|----------|---------------|
| exec.library | `autodocs-3.5/exec-library-*` | `libraries/` | `examples/libraries/` | AllocMem, FreeMem, OpenLibrary, CloseLibrary, SetFunction, Alert, FindTask, AddTask, Signal, Wait |
| dos.library | `autodocs-3.5/dos-library-*` | `libraries/` | `examples/libraries/` | Open, Close, Read, Write, Lock, UnLock, Examine, GetDeviceProc, FreeDeviceProc, DateStamp |
| timer.device | `autodocs-3.5/timer-device-*` | `devices/13-timer-device*` | `examples/devices/13-*` | ReadEClock, GetSysTime, TR_ADDREQUEST, AddTime, SubTime, CmpTime |
| console.device | `autodocs-3.5/console-device-*` | `devices/` | `examples/devices/` | CDInputHandler, RawKeyConvert |
| intuition.library | `autodocs-3.5/intuition-library-*` | `libraries/` | `examples/libraries/` | OpenWindow, CloseWindow, DisplayAlert, SetMenuStrip |
| graphics.library | `autodocs-3.5/graphics-library-*` | `libraries/` | `examples/libraries/` | Move, Draw, Text, SetAPen, RectFill, BltBitMap |
| utility.library | `autodocs-3.5/utility-library-*` | `libraries/` | — | TagInArray, GetTagData, Stricmp, ToUpper |
| ARexx (rexxsyslib) | `autodocs-3.5/rexxsyslib-*` | — | — | CreateRexxMsg, SendRexxCommand |
| Hardware | — | `hardware/` | — | Custom chip registers, CIA, DMA |

## Include File Reference

For struct definitions and constants:

```
Grep pattern="struct EClockVal" path="docs/references/adcd/INCLUDES.json"
```

Or browse by area:

```
Read file_path="docs/references/adcd/autodocs-3.5/include-exec-io-h.md"        # IO structs
Read file_path="docs/references/adcd/autodocs-3.5/include-devices-timer-h.md"   # Timer structs
Read file_path="docs/references/adcd/autodocs-3.5/include-dos-dos-h.md"         # DOS constants
```

## 68k Hardware Reference

For CPU internals, memory map, custom chips, and DMA timing:

```
Read file_path="docs/references/68k-hardware.md"                    # Quick reference
Glob pattern="docs/references/amiga-intern/*.md"                     # Full Amiga Intern book
```

## Common Lookup Patterns

### "How do I open timer.device?"
```
Read docs/references/adcd/devices/13-timer-device.md
Read docs/references/adcd/examples/devices/13-timer-device-*.c
```

### "What fields does FileInfoBlock have?"
```
Grep pattern="FileInfoBlock" path="docs/references/adcd/TYPES.md"
Read docs/references/adcd/autodocs-3.5/include-dos-dos-h.md
```

### "How does SetFunction work?"
```
Read docs/references/adcd/autodocs-3.5/exec-library-setfunction-2.md
```

### "What are the Guru Meditation error codes?"
```
Read docs/references/adcd/libraries/alert-index.md
Read docs/references/crash-patterns.md
```

### "How does the Amiga memory system work?"
```
Read docs/references/adcd/autodocs-3.5/exec-library-allocmem-2.md
Read docs/references/amiga-intern/05-memory-management.md
```

## Important Notes

- **V36 = AmigaOS 2.0**, **V39 = AmigaOS 3.0**, **V40 = AmigaOS 3.1** — check version requirements
- All function offsets (LVOs) are negative: Alert is -108, SetFunction is -420
- Library base pointers (SysBase, DOSBase) must be valid before calling functions
- **proto/*.h** headers provide inline stubs; **clib/*.h** is deprecated
- ADCD examples may have formatting artifacts from markdown conversion — verify against the autodocs
