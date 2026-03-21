---
name: write-arexx
description: Write ARexx scripts for AmigaOS automation, testing, and inter-process communication. Use when creating or modifying .rexx files, writing FS-UAE test harnesses, or automating Amiga workflows.
allowed-tools: Read, Write, Edit, Bash, Grep, Glob
---

# Write ARexx Scripts

You are writing ARexx scripts for AmigaOS 3.x. ARexx is the Amiga's built-in scripting language — think AppleScript but for Amiga. It's string-based, has powerful parsing, and can communicate with any Amiga program that has an ARexx port.

## Reference

Read `docs/references/arexx-reference.md` for the complete language reference. It covers syntax, built-in functions, file I/O, IPC, and testing patterns. **Read it before writing any ARexx code.**

## Mandatory Rules

### 1. First Line Must Be a Comment

Every ARexx script MUST start with a comment on line 1. Without this, AmigaDOS treats it as a shell script, not ARexx:

```rexx
/* my-script.rexx — description */
```

### 2. Use Simple Stem Variables, Not Multi-Level Compounds

ARexx compound variables use a single stem + tail: `name.index`. Multi-level compounds like `tests.i.desc` do NOT work as expected — ARexx resolves only the last tail.

```rexx
/* WRONG — multi-level compound */
tests.i.desc = "hello"    /* Actually sets tests.I.DESC */

/* RIGHT — simple indexed stems */
desc.i = "hello"
cmd.i = "echo hello"
expect.i = "hello"
```

### 3. String Comparison Is Case-Insensitive by Default

ARexx comparison operators (`=`, `~=`, `>`, `<`) are case-insensitive for strings. Use strict operators for case-sensitive comparison:

```rexx
/* Case-insensitive (default) */
IF "Hello" = "hello" THEN SAY "matches"  /* TRUE */

/* Case-sensitive (strict operators) */
IF "Hello" == "hello" THEN SAY "matches"  /* FALSE */
```

### 4. No Short-Circuit Evaluation

ARexx evaluates ALL parts of a compound boolean expression:

```rexx
/* WRONG — will crash if ptr is NULL */
IF ptr ~= '' & LENGTH(ptr) > 0 THEN ...

/* RIGHT — nest the conditions */
IF ptr ~= '' THEN DO
    IF LENGTH(ptr) > 0 THEN ...
END
```

### 5. PULL Reads From Stack First, Not Stdin

`PULL` reads from the ARexx message stack first. Only if the stack is empty does it read from stdin. Use `PARSE PULL` for the same behavior, or `READLN('STDIN')` for explicit stdin reading.

### 6. ADDRESS COMMAND for Shell Commands

Run AmigaDOS commands with `ADDRESS COMMAND`:

```rexx
/* Run a command */
ADDRESS COMMAND 'WORK:grep hello WORK:test.txt >T:output.txt'

/* Check return code */
IF RC > 0 THEN SAY 'Command failed with RC=' || RC

/* Capture output via temp file (no backtick equivalent) */
ADDRESS COMMAND 'WORK:grep -c hello WORK:test.txt >T:count.txt'
IF OPEN('f', 'T:count.txt', 'R') THEN DO
    result = READLN('f')
    CALL CLOSE('f')
END
```

### 7. File I/O Pattern

```rexx
/* Read a file line by line */
IF OPEN('fh', 'WORK:data.txt', 'R') THEN DO
    DO WHILE ~EOF('fh')
        line = READLN('fh')
        /* process line */
    END
    CALL CLOSE('fh')
END

/* Write to a file */
IF OPEN('fh', 'RESULTS:output.txt', 'W') THEN DO
    CALL WRITELN('fh', 'line 1')
    CALL WRITELN('fh', 'line 2')
    CALL CLOSE('fh')
END
```

### 8. Error Handling

```rexx
/* Set error trap */
SIGNAL ON ERROR

/* ... script body ... */

EXIT 0

ERROR:
    SAY 'Error' RC 'at line' SIGL
    EXIT 20
```

### 9. PARSE for String Splitting

PARSE is ARexx's most powerful feature — use it instead of manual string manipulation:

```rexx
/* Split by delimiter */
PARSE VAR line first ':' rest

/* Split by position */
PARSE VAR line name 1 20 age 21 30

/* Split by words */
PARSE VAR line word1 word2 rest

/* Split command output */
PARSE VALUE TIME() WITH hours ':' minutes ':' seconds
```

## Testing Patterns

When writing test harnesses for the FS-UAE testing pipeline:

### TAP Output Format

```rexx
/* TAP (Test Anything Protocol) */
CALL WRITELN('rf', '1..' || testcount)           /* Plan line */
CALL WRITELN('rf', 'ok 1 - test description')    /* Pass */
CALL WRITELN('rf', 'not ok 2 - test description') /* Fail */
CALL WRITELN('rf', '#   expected: foo')           /* Diagnostic */
CALL WRITELN('rf', '#   actual:   bar')           /* Diagnostic */
CALL WRITELN('rf', '# passed: N failed: M total: T') /* Summary */
```

### Test Case File Format

The standard test case format for `test-fsemu-cases.txt`:

```
TEST: description of test
CMD: WORK:program args
EXPECT: expected single-line output
```

### Standard Test Runner Structure

See `toolchain/templates/test-runner.rexx` for the canonical pattern:
1. Read test cases from `WORK:test-cases.txt`
2. Parse into indexed stems (`desc.i`, `cmd.i`, `expect.i`)
3. Run each command with `ADDRESS COMMAND`, redirect output to `T:`
4. Read actual output, compare with expected
5. Write TAP results to `RESULTS:tap-output.txt`
6. Write sentinel file `RESULTS:tests-complete`
7. Call `C:UAEQuit` to shut down the emulator

### Sentinel File Format

```rexx
IF OPEN('sf', 'RESULTS:tests-complete', 'W') THEN DO
    CALL WRITELN('sf', 'TESTS_COMPLETE')
    CALL WRITELN('sf', 'passed=' || passed)
    CALL WRITELN('sf', 'failed=' || failed)
    CALL WRITELN('sf', 'total=' || testcount)
    CALL CLOSE('sf')
END
```

## AmigaOS Path Conventions

- `WORK:` — mounted work volume (port binaries)
- `RESULTS:` — mounted results volume (test output)
- `T:` — temporary files (maps to `RAM:T/`)
- `SYS:` — system volume
- `S:` — scripts directory (`SYS:S/`)
- `C:` — commands directory (`SYS:C/`)
- `NIL:` — /dev/null equivalent
- Use `/` as directory separator within volumes
- No `.` or `..` — use `/` alone for parent directory

## Known Limitations / Gotchas

### 1. No Shell Piping in ADDRESS COMMAND

`ADDRESS COMMAND 'echo "hello" | grep hello'` does **NOT** work. AmigaDOS pipe handling through ARexx's ADDRESS COMMAND is unreliable. Instead, write input to a temp file first, then run the command with the file as input.

```rexx
/* BAD — pipe will not work */
ADDRESS COMMAND 'echo "a:b:c" | cut -d: -f2 >T:out'

/* GOOD — use a temp file */
IF OPEN('tf', 'T:input.txt', 'W') THEN DO
    CALL WRITELN('tf', 'a:b:c')
    CALL CLOSE('tf')
END
ADDRESS COMMAND 'cut -d: -f2 T:input.txt >T:out'
```

### 2. No Command Chaining with && or ;

AmigaDOS does not support `&&` for conditional chaining. The `;` separator may work in some shells but not through ARexx's ADDRESS COMMAND. Run commands as separate ADDRESS COMMAND calls instead.

```rexx
/* BAD — chaining will not work */
ADDRESS COMMAND 'echo >T:f.txt "hi" && cut -c1-2 T:f.txt'

/* GOOD — separate calls */
ADDRESS COMMAND 'echo >T:f.txt "hi"'
ADDRESS COMMAND 'cut -c1-2 T:f.txt >T:out'
```

### 3. Default FAILAT Is 10

Commands returning RC >= 10 trigger ARexx's ERROR condition by default. Programs like `diff` return RC=5 (RETURN_WARN) when files differ, which is fine. But if a command returns RC=10 (RETURN_ERROR), ARexx will jump to the ERROR: label (or abort). Use `OPTIONS FAILAT 21` at the top of test harnesses to prevent this.

```rexx
/* At the top of every test harness */
OPTIONS FAILAT 21
```

### 4. Output Capture with Non-Zero RC

When using `ADDRESS COMMAND cmd '>' outfile`, some ARexx implementations may not write the output file when the command returns non-zero. Use `OPTIONS FAILAT 21` to ensure output is always captured regardless of the command's return code.

### 5. Test Cases Should Use Pre-Created Input Files

For FS-UAE test cases (`test-fsemu-cases.txt`), write test input data to separate files (e.g., `test-grep-input.txt`) that get copied to `WORK:` by the test infrastructure. Do not try to create files on-the-fly in the CMD field.

```
# BAD — trying to create input inline
TEST: grep finds match
CMD: echo "hello world" | WORK:grep hello

# GOOD — use a pre-created input file
TEST: grep finds match
CMD: WORK:grep hello WORK:test-grep-input.txt
```

## Common Gotchas Checklist

Before finishing any ARexx script, verify:

- [ ] First line is a comment (`/* ... */`)
- [ ] No multi-level compound variables (use `name.i` not `obj.i.field`)
- [ ] No short-circuit boolean evaluation (nest IFs instead)
- [ ] All OPEN calls have matching CLOSE calls
- [ ] All temp files in `T:` are cleaned up (`ADDRESS COMMAND 'Delete >NIL: T:file'`)
- [ ] Error handling via `SIGNAL ON ERROR` if the script can fail
- [ ] Exit codes use Amiga conventions: 0=OK, 5=WARN, 10=ERROR, 20=FAIL
- [ ] String comparisons use strict operators (`==`) when case matters
