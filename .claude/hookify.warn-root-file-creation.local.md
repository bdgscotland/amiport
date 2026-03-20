---
name: warn-root-file-creation
enabled: true
event: bash
pattern: >\s*/Users/duncan/Developer/amiport/test_|gcc\s+-o\s*test_|gcc\s+-o\s*/Users/duncan/Developer/amiport/test_
action: block
---

**BLOCKED: Creating test file in project root!**

Your command creates a file in the project root. All test/build artifacts must stay inside `ports/<name>/`.

Fix the output path to point inside the port directory.
