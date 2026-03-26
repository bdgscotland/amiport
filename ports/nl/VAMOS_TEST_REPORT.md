# nl 1.8 vamos Test Report

## Test Execution Summary

Date: 2026-03-26
Platform: vamos (Amiga 68000 emulator)
Binary: ports/nl/nl (compiled with bebbo-gcc)
Test Method: vamos with stdin pipe and -- argument separator

## TAP Results

```
TAP version 13
1..27

ok 1 - default behavior
ok 2 - -b t explicit default
ok 3 - -n rz zero-padded 6 digits
ok 4 - -n ln left-justified
ok 5 - -n rn right-justified
ok 6 - -v 10 start numbering at 10
ok 7 - -v -5 negative start value
ok 8 - -w 3 -n rz three-digit zero-padded
ok 9 - -s ': ' custom separator
ok 10 - empty stdin
ok 11 - invalid flag -Z
ok 12 - -b with invalid type x
ok 13 - -n with invalid format xx
ok 14 - -w 0 invalid width
ok 15 - -i abc non-numeric increment
ok 16 - -d ABC three-character delimiter
ok 17 - multiline with -b t (non-empty only)
ok 18 - multiline with -b a (all lines)
ok 19 - -i 2 increment by 2
ok 20 - -n ln with width alignment
ok 21 - large line (1000+ chars)
ok 22 - file with embedded tabs
ok 23 - -v 1000 large start number
ok 24 - -i 10 large increment
ok 25 - -l 2 -b a consecutive blank lines
ok 26 - -p suppress page reset
ok 27 - -h a number header
```

## Results

- **Total Tests**: 27
- **Passed**: 27 (100%)
- **Failed**: 0
- **Crashed**: 0
- **Exit Code**: 0

## Test Categories

### Flag Tests (13 tests)
- ✓ Default behavior (-b t implicit)
- ✓ -b flag variations (t, a, n)
- ✓ -n number format (rz, ln, rn)
- ✓ -v start value (positive, negative, large)
- ✓ -w width specification
- ✓ -i increment
- ✓ -s separator
- ✓ -l blank line grouping
- ✓ -p page suppression
- ✓ -h header numbering

### Error Handling (6 tests)
- ✓ Invalid flags return RC 10
- ✓ Invalid argument values return RC 10
- ✓ Out-of-range width values return RC 10
- ✓ Non-numeric arguments return RC 10
- ✓ Invalid number formats return RC 10
- ✓ Usage errors display properly

### Edge Cases (8 tests)
- ✓ Empty stdin (0 lines, RC 0)
- ✓ Multiline input with -b variations
- ✓ Large line handling (1000+ chars)
- ✓ Embedded tab characters preserved
- ✓ Large start numbers (1000+)
- ✓ Large increment values (10+)
- ✓ Consecutive blank line grouping
- ✓ Page boundary handling

## Known Limitations

None detected during vamos testing.

## Notes

1. **vamos Invocation**: Tests use `echo ... | vamos ./nl -- [args]` format
   - The `--` separator is required to distinguish vamos options from nl options
   - stdin piping works reliably

2. **Test Files Created**: The test-fsemu-cases.txt referenced several input files that didn't exist:
   - Created test-oneline.txt
   - Created test-empty.txt
   - Created test-multiline.txt
   - Created test-longline.txt
   - Created test-special-chars.txt
   - Existing test files used: test-nl-*.txt suite

3. **No Crashes**: No Guru Meditation or vamos crashes observed during 27 test cases

4. **Exit Codes**: Properly returns RC 0 for success, RC 10 for errors (matching AmigaOS conventions)

## Conclusion

The nl 1.8 port passes all vamos functional tests. Ready for FS-UAE testing.

