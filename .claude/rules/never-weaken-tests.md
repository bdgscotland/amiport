# Never Weaken Tests — Hard Rule

## The Rule

**NEVER change a test assertion to make a failing test pass. Fix the code instead.**

Specifically:
- **NEVER replace `EXPECT:` with `EXPECT_CONTAINS:`** to work around output differences
- **NEVER broaden a substring match** to ignore precision/formatting errors
- **NEVER remove a failing test** to get to green
- **NEVER reduce the expected value precision** to paper over floating point bugs

## What To Do Instead

When a test fails:

1. **Investigate the root cause** — is it a code bug, a shim/libnix bug, or a wrong test expectation?
2. **If the code is wrong**: fix the code, rebuild, retest
3. **If the test expectation is wrong** (verified by running the native tool): fix the test to match the correct native output
4. **If it's a shim/libnix limitation** that can't be fixed now: leave the strict assertion in place, document the failure in PORT.md under "Known Bugs", and do NOT proceed to Stage 6. A red test is better than a lying green test.

## Deterministic Gate

The `/port-project` pipeline MUST NOT proceed past Stage 5 if any functional test fails. The only acceptable paths forward are:
- Fix the bug and retest
- Remove the test entirely with a documented justification in PORT.md (e.g., "test depends on feature not available on AmigaOS")
- Leave the test failing and halt the pipeline — document as WIP

## Why This Rule Exists

During the awk port (2026-03-25), 9 tests were weakened from exact match to substring match to hide a libnix `%g` formatting bug. This masked real functional bugs including incorrect floating point results (`fib(20)` returning `6764.999...` instead of `6765`). The tests went green but the port was broken.
