---
name: xdftool write requires delete-first
description: xdftool write fails with "Name already exists" if the target file is present — must delete before writing
type: project
---

`xdftool <hdf> write <src> <dst>` fails silently (or with "Name already exists" FSError) when the destination already exists in the HDF. This causes `test-fsemu.sh` to get stuck when a previous run was interrupted (leaving the test User-Startup in the HDF).

**Why:** xdftool does not support an overwrite/replace mode. Write operations are create-only.

**Fix applied (2026-03-21):** `scripts/test-fsemu.sh` now runs `xdftool delete S/User-Startup` before every `xdftool write` in both the install path and the cleanup/restore path.

**How to apply:** Whenever using `xdftool write` in any script, always precede it with `xdftool delete <dst> 2>/dev/null || true` to clear the target first.
