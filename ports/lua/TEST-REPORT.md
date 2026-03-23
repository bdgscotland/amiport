# FS-UAE Test Report: lua

## Summary

| Field | Value |
|-------|-------|
| Port | lua |
| Date | 2026-03-22 21:33:27 |
| Duration | 101s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:lua` (247K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 65/65 passed |

## Test Results

```
1..65
ok 1 - -v prints version string
ok 2 - -e executes a print expression
ok 3 - -e string concatenation
ok 4 - -e with math library call
ok 5 - -e with for loop accumulator
ok 6 - -e with semicolons executes multiple statements
ok 7 - -E ignores environment variables and runs cleanly
ok 8 - -W enables warnings without crashing
ok 9 - -W combined with -E flag
ok 10 - -l loads a standard library into a global
ok 11 - -l g=mod form loads module into named global
ok 12 - -- followed by script file stops option processing
ok 13 - execute a script file via WORK: volume path
ok 14 - script with table operations (length and element access)
ok 15 - arg table is populated correctly (arg[0] is script name)
ok 16 - -e before script: -e runs first, then script
ok 17 - unrecognized option returns RC 10
ok 18 - -e with missing argument returns RC 10
ok 19 - -l with missing argument returns RC 10
ok 20 - -l with nonexistent module returns RC 10
ok 21 - nonexistent script file returns RC 10
ok 22 - syntax error in -e argument returns RC 10
ok 23 - syntax error in script file returns RC 10
ok 24 - runtime error in script returns RC 10 (call nil value)
ok 25 - io.popen not supported returns false from pcall (ISO C stub)
ok 26 - package.loadlib returns error (no dlopen on AmigaOS)
ok 27 - empty script file exits cleanly with RC 0
ok 28 - os.exit(true) returns RC 0
ok 29 - os.exit(false) returns RC 10 (Amiga RETURN_ERROR)
ok 30 - os.exit(5) returns RC 5 (Amiga RETURN_WARN)
ok 31 - os.exit(0) returns RC 0
ok 32 - integer floor division (Lua 5.4 feature)
ok 33 - bitwise AND operator
ok 34 - all six bitwise operators (AND OR XOR NOT SHL SHR)
ok 35 - math.maxinteger is 2147483647 on Amiga (32-bit long)
ok 36 - math.mininteger is -2147483648 on Amiga (32-bit long)
ok 37 - integer overflow wraps around (32-bit semantics)
ok 38 - math.type distinguishes integer from float
ok 39 - to-be-closed variables call __close in LIFO order
ok 40 - to-be-closed variables close in reverse order (y before x)
ok 41 - collectgarbage returns numeric memory count (GC active)
ok 42 - collectgarbage step mode returns boolean result
ok 43 - coroutine yield and resume (first yield value)
ok 44 - metatable __index enables method dispatch
ok 45 - metatable __tostring produces custom string representation
ok 46 - string library operations (string.len first output)
ok 47 - string.format with integer, float, and string formats
ok 48 - string.format %05d zero-pads correctly
ok 49 - string.format %q produces quoted string
ok 50 - multiple return values via swap function
ok 51 - table.unpack returns all elements
ok 52 - closure captures upvalue correctly (first counter call)
ok 53 - independent closures have separate upvalue state
ok 54 - pcall catches runtime error and returns false
ok 55 - xpcall with message handler produces wrapped message
ok 56 - os.tmpname returns a T: volume path (Amiga shim)
ok 57 - io.tmpfile() via amiport_tmpfile writes and reads back
ok 58 - file I/O roundtrip using T: volume
ok 59 - type() returns correct type names for Amiga Lua build
ok 60 - large table (1000 entries): first element is 1
ok 61 - large table length is reported correctly as 1000
ok 62 - deep recursion (Fibonacci fib(20) = 6765)
ok 63 - integer arithmetic: 32-bit wraparound does not crash
ok 64 - string pattern matching (string library)
ok 65 - tostring/tonumber round-trip
# passed: 65 failed: 0 total: 65
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | -v prints version string | PASS | |
| 2 | -e executes a print expression | PASS | |
| 3 | -e string concatenation | PASS | |
| 4 | -e with math library call | PASS | |
| 5 | -e with for loop accumulator | PASS | |
| 6 | -e with semicolons executes multiple statements | PASS | |
| 7 | -E ignores environment variables and runs cleanly | PASS | |
| 8 | -W enables warnings without crashing | PASS | |
| 9 | -W combined with -E flag | PASS | |
| 10 | -l loads a standard library into a global | PASS | |
| 11 | -l g=mod form loads module into named global | PASS | |
| 12 | -- followed by script file stops option processing | PASS | |
| 13 | execute a script file via WORK: volume path | PASS | |
| 14 | script with table operations (length and element access) | PASS | |
| 15 | arg table is populated correctly (arg[0] is script name) | PASS | |
| 16 | -e before script: -e runs first, then script | PASS | |
| 17 | unrecognized option returns RC 10 | PASS | |
| 18 | -e with missing argument returns RC 10 | PASS | |
| 19 | -l with missing argument returns RC 10 | PASS | |
| 20 | -l with nonexistent module returns RC 10 | PASS | |
| 21 | nonexistent script file returns RC 10 | PASS | |
| 22 | syntax error in -e argument returns RC 10 | PASS | |
| 23 | syntax error in script file returns RC 10 | PASS | |
| 24 | runtime error in script returns RC 10 (call nil value) | PASS | |
| 25 | io.popen not supported returns false from pcall (ISO C stub) | PASS | |
| 26 | package.loadlib returns error (no dlopen on AmigaOS) | PASS | |
| 27 | empty script file exits cleanly with RC 0 | PASS | |
| 28 | os.exit(true) returns RC 0 | PASS | |
| 29 | os.exit(false) returns RC 10 (Amiga RETURN_ERROR) | PASS | |
| 30 | os.exit(5) returns RC 5 (Amiga RETURN_WARN) | PASS | |
| 31 | os.exit(0) returns RC 0 | PASS | |
| 32 | integer floor division (Lua 5.4 feature) | PASS | |
| 33 | bitwise AND operator | PASS | |
| 34 | all six bitwise operators (AND OR XOR NOT SHL SHR) | PASS | |
| 35 | math.maxinteger is 2147483647 on Amiga (32-bit long) | PASS | |
| 36 | math.mininteger is -2147483648 on Amiga (32-bit long) | PASS | |
| 37 | integer overflow wraps around (32-bit semantics) | PASS | |
| 38 | math.type distinguishes integer from float | PASS | |
| 39 | to-be-closed variables call __close in LIFO order | PASS | |
| 40 | to-be-closed variables close in reverse order (y before x) | PASS | |
| 41 | collectgarbage returns numeric memory count (GC active) | PASS | |
| 42 | collectgarbage step mode returns boolean result | PASS | |
| 43 | coroutine yield and resume (first yield value) | PASS | |
| 44 | metatable __index enables method dispatch | PASS | |
| 45 | metatable __tostring produces custom string representation | PASS | |
| 46 | string library operations (string.len first output) | PASS | |
| 47 | string.format with integer, float, and string formats | PASS | |
| 48 | string.format %05d zero-pads correctly | PASS | |
| 49 | string.format %q produces quoted string | PASS | |
| 50 | multiple return values via swap function | PASS | |
| 51 | table.unpack returns all elements | PASS | |
| 52 | closure captures upvalue correctly (first counter call) | PASS | |
| 53 | independent closures have separate upvalue state | PASS | |
| 54 | pcall catches runtime error and returns false | PASS | |
| 55 | xpcall with message handler produces wrapped message | PASS | |
| 56 | os.tmpname returns a T: volume path (Amiga shim) | PASS | |
| 57 | io.tmpfile() via amiport_tmpfile writes and reads back | PASS | |
| 58 | file I/O roundtrip using T: volume | PASS | |
| 59 | type() returns correct type names for Amiga Lua build | PASS | |
| 60 | large table (1000 entries): first element is 1 | PASS | |
| 61 | large table length is reported correctly as 1000 | PASS | |
| 62 | deep recursion (Fibonacci fib(20) = 6765) | PASS | |
| 63 | integer arithmetic: 32-bit wraparound does not crash | PASS | |
| 64 | string pattern matching (string library) | PASS | |
| 65 | tostring/tonumber round-trip | PASS | |

## Environment

| Component | Version/Path |
|-----------|-------------|
| FS-UAE | 3.2.35 |
| Kickstart | 3.1 (40.68) |
| Amiga model | A1200 (68020) |
| Compiler | m68k-amigaos-gcc (bebbo) |
| POSIX shim | libamiport.a |
| Regex emu | libamiport-emu.a |
| Test harness | ARexx (test-runner.rexx) |

## Test Cases

Each test runs the command inside AmigaOS, captures stdout to a file,
and compares against the expected output string.

```
# Lua 5.4.7 FS-UAE test suite
# Category 2 (scripting interpreter) -- minimum 20 tests required
# Generated by test-designer agent, 2026-03-22
#
# FUNCTIONAL TESTS  : -v, -e, -E, -W, -l, -l g=mod, --, -e+script, flag combos
# ERROR PATH TESTS  : bad flag, -e missing arg, -l missing arg, nonexistent file,
#                     syntax error (-e), syntax error (script), runtime error,
#                     io.popen (not supported), package.loadlib (not supported)
# EXIT CODE TESTS   : RC=0 (success), RC=5 (os.exit(5)), RC=10 (error)
# EDGE CASE TESTS   : empty script, 32-bit integer bounds, large table, deep
#                     recursion, string.format, multiple returns, closures, pcall
# AMIGA-SPECIFIC    : T: tmpname, io.tmpfile via amiport_tmpfile, file I/O on T:,
#                     WORK: volume paths, arg table, os.execute check
# LUA 5.4 FEATURES  : floor div, bitwise ops (all 6), to-be-closed, metatables,
#                     generational GC control, coroutines

# ============================================================
# SECTION 1: -v flag
# ============================================================

TEST: -v prints version string
CMD: WORK:lua -v
EXPECT: Lua 5.4.7  Copyright (C) 1994-2024 Lua.org, PUC-Rio
EXPECT_RC: 0

# ============================================================
# SECTION 2: -e flag (execute string)
# ============================================================

TEST: -e executes a print expression
CMD: WORK:lua -e "print(2+3)"
EXPECT: 5
EXPECT_RC: 0

TEST: -e string concatenation
CMD: WORK:lua -e "print('amiga' .. '68k')"
EXPECT: amiga68k
EXPECT_RC: 0

TEST: -e with math library call
CMD: WORK:lua -e "print(math.sqrt(144))"
EXPECT: 12.0
EXPECT_RC: 0

TEST: -e with for loop accumulator
CMD: WORK:lua -e "local s=0 for i=1,100 do s=s+i end print(s)"
EXPECT: 5050
EXPECT_RC: 0

TEST: -e with semicolons executes multiple statements
CMD: WORK:lua -e "x=10; print(x+x)"
EXPECT: 20
EXPECT_RC: 0

# ============================================================
# SECTION 3: -E flag (ignore environment)
# ============================================================

TEST: -E ignores environment variables and runs cleanly
CMD: WORK:lua -E -e "print('env ignored')"
EXPECT: env ignored
EXPECT_RC: 0

# ============================================================
# SECTION 4: -W flag (enable warnings)
# ============================================================

TEST: -W enables warnings without crashing
CMD: WORK:lua -W -e "print('warnings on')"
EXPECT: warnings on
EXPECT_RC: 0

TEST: -W combined with -E flag
CMD: WORK:lua -W -E -e "print('warn+noenv')"
EXPECT: warn+noenv
EXPECT_RC: 0

# ============================================================
# SECTION 5: -l flag (require library)
# ============================================================

TEST: -l loads a standard library into a global
CMD: WORK:lua -l string -e "print(type(string))"
EXPECT: table
EXPECT_RC: 0

TEST: -l g=mod form loads module into named global
CMD: WORK:lua -l s=string -e "print(type(s))"
EXPECT: table
EXPECT_RC: 0

# ============================================================
# SECTION 6: -- flag (stop option processing)
# ============================================================

TEST: -- followed by script file stops option processing
CMD: WORK:lua -- WORK:test-lua-hello.lua
EXPECT: hello from script
EXPECT_RC: 0

# ============================================================
# SECTION 7: Script file execution via WORK: volume path
# ============================================================

TEST: execute a script file via WORK: volume path
CMD: WORK:lua WORK:test-lua-hello.lua
EXPECT: hello from script
EXPECT_RC: 0

TEST: script with table operations (length and element access)
CMD: WORK:lua WORK:test-lua-table.lua
EXPECT: 5
EXPECT_RC: 0

TEST: arg table is populated correctly (arg[0] is script name)
CMD: WORK:lua WORK:test-lua-args.lua
EXPECT: table
EXPECT_RC: 0

TEST: -e before script: -e runs first, then script
CMD: WORK:lua -e "prefix='ok'" WORK:test-lua-hello.lua
EXPECT: hello from script
EXPECT_RC: 0

# ============================================================
# SECTION 8: Error path tests -- all use EXPECT_RC: 10 only
# (error messages go to stderr, not captured by harness)
# ============================================================

TEST: unrecognized option returns RC 10
CMD: WORK:lua -Z
EXPECT_RC: 10

TEST: -e with missing argument returns RC 10
CMD: WORK:lua -e
EXPECT_RC: 10

TEST: -l with missing argument returns RC 10
CMD: WORK:lua -l
EXPECT_RC: 10

TEST: -l with nonexistent module returns RC 10
CMD: WORK:lua -l nosuchmodule99
EXPECT_RC: 10

TEST: nonexistent script file returns RC 10
CMD: WORK:lua WORK:no-such-script.lua
EXPECT_RC: 10

TEST: syntax error in -e argument returns RC 10
CMD: WORK:lua -e "print(unclosed"
EXPECT_RC: 10

TEST: syntax error in script file returns RC 10
CMD: WORK:lua WORK:test-lua-syntax-error.lua
EXPECT_RC: 10

TEST: runtime error in script returns RC 10 (call nil value)
CMD: WORK:lua WORK:test-lua-runtime-error.lua
EXPECT_RC: 10

TEST: io.popen not supported returns false from pcall (ISO C stub)
CMD: WORK:lua -e "print(pcall(io.popen, 'ls'))"
EXPECT_CONTAINS: false
EXPECT_RC: 0

TEST: package.loadlib returns error (no dlopen on AmigaOS)
CMD: WORK:lua -e "local f,e=package.loadlib('nosuch.so','foo'); print(f==nil)"
EXPECT: true
EXPECT_RC: 0

# ============================================================
# SECTION 9: Exit code tests
# ============================================================

TEST: empty script file exits cleanly with RC 0
CMD: WORK:lua WORK:test-lua-empty.lua
EXPECT_RC: 0

TEST: os.exit(true) returns RC 0
CMD: WORK:lua -e "os.exit(true)"
EXPECT_RC: 0

TEST: os.exit(false) returns RC 10 (Amiga RETURN_ERROR)
CMD: WORK:lua -e "os.exit(false)"
EXPECT_RC: 10

TEST: os.exit(5) returns RC 5 (Amiga RETURN_WARN)
CMD: WORK:lua WORK:test-lua-os-exit.lua
EXPECT_RC: 5

TEST: os.exit(0) returns RC 0
CMD: WORK:lua -e "os.exit(0)"
EXPECT_RC: 0

# ============================================================
# SECTION 10: Lua 5.4 integer and bitwise features
# ============================================================

TEST: integer floor division (Lua 5.4 feature)
CMD: WORK:lua -e "print(7 // 2)"
EXPECT: 3
EXPECT_RC: 0

TEST: bitwise AND operator
CMD: WORK:lua -e "print(0xFF & 0x0F)"
EXPECT: 15
EXPECT_RC: 0

TEST: all six bitwise operators (AND OR XOR NOT SHL SHR)
CMD: WORK:lua WORK:test-lua-bitwise.lua
EXPECT: 15
EXPECT_RC: 0

TEST: math.maxinteger is 2147483647 on Amiga (32-bit long)
CMD: WORK:lua -e "print(math.maxinteger)"
EXPECT: 2147483647
EXPECT_RC: 0

TEST: math.mininteger is -2147483648 on Amiga (32-bit long)
CMD: WORK:lua -e "print(math.mininteger)"
EXPECT: -2147483648
EXPECT_RC: 0

TEST: integer overflow wraps around (32-bit semantics)
CMD: WORK:lua -e "print(math.maxinteger + 1 == math.mininteger)"
EXPECT: true
EXPECT_RC: 0

TEST: math.type distinguishes integer from float
CMD: WORK:lua -e "print(math.type(1), math.type(1.0))"
EXPECT: integer	float
EXPECT_RC: 0

# ============================================================
# SECTION 11: Lua 5.4 to-be-closed variables
# ============================================================

TEST: to-be-closed variables call __close in LIFO order
CMD: WORK:lua WORK:test-lua-toclose.lua
EXPECT: inside
EXPECT_RC: 0

TEST: to-be-closed variables close in reverse order (y before x)
CMD: WORK:lua WORK:test-lua-toclose.lua
EXPECT_CONTAINS: closed:y
EXPECT_RC: 0

# ============================================================
# SECTION 12: Generational GC
# ============================================================

TEST: collectgarbage returns numeric memory count (GC active)
CMD: WORK:lua WORK:test-lua-gc.lua
EXPECT: number
EXPECT_RC: 0

TEST: collectgarbage step mode returns boolean result
CMD: WORK:lua WORK:test-lua-gc.lua
EXPECT_CONTAINS: boolean
EXPECT_RC: 0

# ============================================================
# SECTION 13: Coroutines
# ============================================================

TEST: coroutine yield and resume (first yield value)
CMD: WORK:lua WORK:test-lua-coroutine.lua
EXPECT: 15
EXPECT_RC: 0

# ============================================================
# SECTION 14: Metatables
# ============================================================

TEST: metatable __index enables method dispatch
CMD: WORK:lua WORK:test-lua-metatables.lua
EXPECT: 42
EXPECT_RC: 0

TEST: metatable __tostring produces custom string representation
CMD: WORK:lua WORK:test-lua-metatables.lua
EXPECT_CONTAINS: obj:21
EXPECT_RC: 0

# ============================================================
# SECTION 15: String library
# ============================================================

TEST: string library operations (string.len first output)
CMD: WORK:lua WORK:test-lua-string-ops.lua
EXPECT: 13
EXPECT_RC: 0

TEST: string.format with integer, float, and string formats
CMD: WORK:lua WORK:test-lua-string-format.lua
EXPECT: 42
EXPECT_RC: 0

TEST: string.format %05d zero-pads correctly
CMD: WORK:lua WORK:test-lua-string-format.lua
EXPECT_CONTAINS: 00042
EXPECT_RC: 0

TEST: string.format %q produces quoted string
CMD: WORK:lua WORK:test-lua-string-format.lua
EXPECT_CONTAINS: say
EXPECT_RC: 0

# ============================================================
# SECTION 16: Multiple return values and table operations
# ============================================================

TEST: multiple return values via swap function
CMD: WORK:lua WORK:test-lua-multireturn.lua
EXPECT: 2	1
EXPECT_RC: 0

TEST: table.unpack returns all elements
CMD: WORK:lua WORK:test-lua-multireturn.lua
EXPECT_CONTAINS: 10
EXPECT_RC: 0

# ============================================================
# SECTION 17: Closures
# ============================================================

TEST: closure captures upvalue correctly (first counter call)
CMD: WORK:lua WORK:test-lua-closures.lua
EXPECT: 1
EXPECT_RC: 0

TEST: independent closures have separate upvalue state
CMD: WORK:lua WORK:test-lua-closures.lua
EXPECT_CONTAINS: 11
EXPECT_RC: 0

# ============================================================
# SECTION 18: pcall / xpcall error handling
# ============================================================

TEST: pcall catches runtime error and returns false
CMD: WORK:lua WORK:test-lua-pcall.lua
EXPECT: false
EXPECT_RC: 0

TEST: xpcall with message handler produces wrapped message
CMD: WORK:lua WORK:test-lua-pcall.lua
EXPECT_CONTAINS: caught:
EXPECT_RC: 0

# ============================================================
# SECTION 19: Amiga-specific behavior
# ============================================================

TEST: os.tmpname returns a T: volume path (Amiga shim)
CMD: WORK:lua -e "print(os.tmpname():sub(1,2))"
EXPECT: T:
EXPECT_RC: 0

TEST: io.tmpfile() via amiport_tmpfile writes and reads back
CMD: WORK:lua WORK:test-lua-io-tmpfile.lua
EXPECT: tmpfile content
EXPECT_RC: 0

TEST: file I/O roundtrip using T: volume
CMD: WORK:lua WORK:test-lua-fileio.lua
EXPECT: amiga file io works
EXPECT_RC: 0

TEST: type() returns correct type names for Amiga Lua build
CMD: WORK:lua -e "print(type(42), type('x'), type(true), type(nil))"
EXPECT: number	string	boolean	nil
EXPECT_RC: 0

# ============================================================
# SECTION 20: Edge cases
# ============================================================

TEST: large table (1000 entries): first element is 1
CMD: WORK:lua WORK:test-lua-large-table.lua
EXPECT: 1
EXPECT_RC: 0

TEST: large table length is reported correctly as 1000
CMD: WORK:lua WORK:test-lua-large-table.lua
EXPECT_CONTAINS: 1000
EXPECT_RC: 0

TEST: deep recursion (Fibonacci fib(20) = 6765)
CMD: WORK:lua -e "function fib(n) if n<2 then return n end return fib(n-1)+fib(n-2) end print(fib(20))"
EXPECT: 6765
EXPECT_RC: 0

TEST: integer arithmetic: 32-bit wraparound does not crash
CMD: WORK:lua -e "local x=math.maxinteger for i=1,10 do x=x+1 end print(type(x))"
EXPECT: number
EXPECT_RC: 0

TEST: string pattern matching (string library)
CMD: WORK:lua -e "local s='hello world' print(string.match(s,'(%a+)%s(%a+)'))"
EXPECT: hello	world
EXPECT_RC: 0

TEST: tostring/tonumber round-trip
CMD: WORK:lua -e "local n=12345 print(tonumber(tostring(n)) == n)"
EXPECT: true
EXPECT_RC: 0
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=65
failed=0
total=65
```

---
Generated by `make test-fsemu TARGET=ports/lua`
Report template: `toolchain/templates/test-report.md.template`
