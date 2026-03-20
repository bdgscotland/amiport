# CIDR-004: Interactive Emulator Testing Workflow

## Status

Exploring (partially implemented)

## Date

2026-03-20

## Idea

While vamos handles automated CLI testing, some scenarios need a full AmigaOS environment — GUI programs, hardware-dependent code, or simply wanting to see the port running on a "real" Amiga. An integrated emulator workflow would let users go from source to interactive testing in one pipeline.

## Current Implementation

- `make install-emu` — copies all built binaries (ports + examples) to `build/amiga/`
- `make emu` — launches FS-UAE with `build/amiga/` mounted as `WORK:`
- FS-UAE config at `toolchain/configs/amiport-test.fs-uae` — A1200, Kickstart 3.1
- In the Amiga shell, run `WORK:cal` or any other built program

## Prerequisites (user must provide)

- FS-UAE: `brew install fs-uae`
- Kickstart 3.1 ROM: purchase from amigaforever.com (~$10), place in `~/Documents/FS-UAE/Kickstarts/`
- Workbench 3.1 (optional): for a full desktop environment with Shell

## Future Exploration

### Bootable test environment
Create a minimal bootable hard drive directory with just enough AmigaOS files (L/, S/, C/, DEVS/, LIBS/) to boot to a Shell prompt without needing Workbench disks. The startup-sequence could automatically list available programs.

### Automated FS-UAE testing
Use FS-UAE's built-in Lua scripting or input recording to automate GUI testing — launch the emulator, run a program, capture a screenshot, compare output. This would extend the test pipeline beyond CLI tools.

### ADF generation
For distribution to real Amiga users without hard drive setups, generate bootable ADF floppy images containing the port. Tools like `xdftool` (from amitools) can create these programmatically.

### Integration with /test-amiga skill
The test-amiga skill currently only knows about vamos. It could detect when a port uses unsupported libraries (intuition, graphics) and automatically suggest FS-UAE testing instead, generating the appropriate config.

## Why Not Now

The current vamos pipeline handles all CLI testing needs. FS-UAE is a nice-to-have for manual verification and demos. The bootable environment and ADF generation are valuable but depend on having Workbench files, which have licensing implications.
