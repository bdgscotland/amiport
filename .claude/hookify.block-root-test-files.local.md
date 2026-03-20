---
name: block-root-test-files
enabled: true
event: file
conditions:
  - field: file_path
    operator: regex_match
    pattern: ^(/Users/duncan/Developer/amiport/)?test_[^/]+$
action: block
---

**BLOCKED: Test file in project root!**

You are creating a test file in the project root directory. This is not allowed.

All test files, build artifacts, and temporary files must go inside the port directory (`ports/<name>/`).

Move this file to the appropriate `ports/<name>/` directory.
