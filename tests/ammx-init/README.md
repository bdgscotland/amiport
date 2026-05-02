# amiport_ammx_init Smoke Test

## Purpose

This test verifies the `amiport_ammx_init()` function from `lib/posix-shim/include/amiport/ammx.h`:

- Confirms that the function initializes AMMX2 context-switch handling
- Verifies that the caching mechanism works (second call returns cached value)
- Validates behavior on systems with and without Apollo 68080 hardware

## Expected Output

### On vamos / FS-UAE (without Apollo emulation)

```
amiport_ammx_init test
======================
First call:  rc=1 (no Apollo 68080 -- expected on stock 68k)
Second call cache check: PASS
```

**Exit code:** 0

The test returns rc=1 because vamos does not set the `AFF_68080` bit in `ExecBase->AttnFlags`. This is the expected behavior on non-Apollo systems.

### On real Apollo A6000

```
amiport_ammx_init test
======================
First call:  rc=0 (SUCCESS -- AMMX2 enabled)
Second call cache check: PASS
```

**Exit code:** 0

The test returns rc=0 on real Apollo hardware when vampire.resource version >= 45 and AMMX2 context-switch support is successfully enabled.

## Return Code Documentation

The test prints human-readable descriptions for each return code:

- `rc=0` — SUCCESS: AMMX2 enabled (Apollo A6000 only)
- `rc=1` — No Apollo 68080 detected (expected on vamos, FS-UAE, stock 68k)
- `rc=2` — vampire.resource missing (expected on non-Vampire systems)
- `rc=3` — vampire.resource too old (V < 45)
- `rc=4` — V_EnableAMMX failed (driver issue on Apollo)

## How to Run

### On vamos (Project Build System)

```bash
make -C tests/ammx-init run
```

### On real A6000 (Task 7 — Deferred)

Push the binary via amigactl:

```bash
python3 -m amigactl --host 192.168.1.215 put tests/ammx-init/test_ammx_init C:test_ammx_init
python3 -m amigactl --host 192.168.1.215 run C:test_ammx_init
```

## Notes

- The test is compiled with `-m68040 -m68881` to match the production Apollo target (NetSurf on Vampire).
- The test deliberately does NOT assert a specific rc value; different rc values are valid depending on the target system.
- The cache check (rc1 == rc2) is mandatory — if the second call returns a different value, that's a real bug.
- The `amiport_ammx_status()` function is also validated to confirm it returns the same cached value.

## Related

- `lib/posix-shim/include/amiport/ammx.h` — Function prototypes
- `lib/posix-shim/src/ammx_init.c` — Implementation
- `docs/references/ammx/` — Apollo AMMX technical reference material
- NetSurf Phase B Task 7 (deferred) — Real A6000 hardware validation
