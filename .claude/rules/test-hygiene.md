# Test Hygiene — No Root Slop

## Hard Rules

1. **NEVER create files in the project root** during porting, building, or testing. This includes test binaries, test input files, temporary files, native comparison builds, and any other artifacts. All files go inside the port's directory (`ports/<name>/`).

2. **All test artifacts go in a temporary directory.** Use `T:` on AmigaOS or create files inside the port directory. Never use `/tmp` or the project root.

3. **Clean up after every test run.** Remove test input files, test output files, and native comparison binaries when done. If a test creates files, the test must delete them.

4. **When dispatching agents** (build-manager, test-runner, etc.), explicitly instruct them: "Do not create any files outside `ports/<name>/`. Clean up all test artifacts when done."

## Why This Matters

Stray files in the project root are noise. They get accidentally committed, confuse `git status`, and make the workspace messy. This project has multiple agents operating concurrently — without this rule, every agent leaves debris behind.
