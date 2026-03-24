# Test Hygiene — No Root Slop

## Hard Rules

1. **NEVER create files in the project root** during porting, building, or testing. This includes test binaries, test input files, temporary files, native comparison builds, and any other artifacts. All files go inside the port's directory (`ports/<name>/`).

2. **All test artifacts go in a temporary directory.** Use `T:` on AmigaOS or create files inside the port directory. Never use `/tmp` or the project root.

3. **Clean up after every test run.** Remove test input files, test output files, and native comparison binaries when done. If a test creates files, the test must delete them.

4. **When dispatching agents** (build-manager, test-runner, etc.), explicitly instruct them: "Do not create any files outside `ports/<name>/`. Clean up all test artifacts when done."

## Compiled Binaries — Keep Them

5. **Compiled Amiga binaries SHOULD be committed.** The built `ports/<name>/<name>` binary is a distributable artifact — commit it alongside the source. Users cloning the repo get working binaries without needing the cross-compiler toolchain. Only exclude profiling artifacts (`gmon.out`) and native comparison binaries (`*_native`).

## Test File Organization

6. **Functional and visual tests MUST be in separate files.** `test-fsemu-cases.txt` contains non-interactive `TEST:` blocks and functional `ITEST:` blocks. `test-fsemu-visual-cases.txt` contains `SCRAPE` visual verification tests (ADR-024). Never mix them — they run as separate FS-UAE passes because resource exhaustion at ~13 ITESTs is a hard wall.

## Why This Matters

Stray files in the project root are noise. They get accidentally committed, confuse `git status`, and make the workspace messy. This project has multiple agents operating concurrently — without this rule, every agent leaves debris behind. But compiled Amiga binaries are the whole point of the project — they should be checked in.
