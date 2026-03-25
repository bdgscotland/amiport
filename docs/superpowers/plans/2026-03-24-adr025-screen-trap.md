# ADR-025: FS-UAE Custom Trap for Interactive Visual Verification

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a custom trap handler to the forked FS-UAE that reads ConUnit screen state from emulated memory, fix mg cursor keys, and prove it works with a visual test.

**Architecture:** New trap mode 150 in FS-UAE's mousehack_done dispatcher reads cursor position + screen dimensions from a ConUnit in emulated memory, writes them to a host-side file. A small Amiga-side binary (ScreenRead) triggers the trap. The mg cursor key bug (CSI 0x9B not translated to ESC [) is fixed in kbd.c. The proving test injects DOWN arrow in mg and asserts cursor moved via the new trap.

**Tech Stack:** C++ (FS-UAE), C89 (Amiga binaries), Python (verify-screen.py), ARexx (test harness)

**Key Reference Files:**
- `~/Developer/fs-uae-3.2.35/src/filesys.cpp:8344` — mousehack_done trap dispatcher
- `~/Developer/fs-uae-3.2.35/src/consolehook.cpp` — existing per-unit ANSI capture
- `docs/references/adcd/autodocs-3.5/include-devices-conunit-h.md` — ConUnit struct layout
- `ports/mg/ported/kbd.c:78` — getkey() with METABIT handler
- `ports/mg/ported/ttyio.c:235` — ttgetc() raw input
- `toolchain/keyinject/` — reference for Amiga-side helper binary pattern
- `scripts/test-fsemu.sh` — FS-UAE test runner
- `scripts/verify-screen.py` — screen verification

**ConUnit Field Offsets (from ADCD conunit.h):**
```
MsgPort cu_MP          offset 0   (34 bytes)
Window *cu_Window      offset 34  (4 bytes)
WORD cu_XCP            offset 38  (character position X)
WORD cu_YCP            offset 40  (character position Y)
WORD cu_XMax           offset 42
WORD cu_YMax           offset 44
WORD cu_XRSize         offset 46
WORD cu_YRSize         offset 48
WORD cu_XROrigin       offset 50
WORD cu_YROrigin       offset 52
WORD cu_XRExtant       offset 54
WORD cu_YRExtant       offset 56
WORD cu_XMinShrink     offset 58
WORD cu_YMinShrink     offset 60
WORD cu_XCCP           offset 62  (cursor col - the one we want)
WORD cu_YCCP           offset 64  (cursor row - the one we want)
```

---

## Task 1: Fix mg Cursor Keys (CSI Translation)

**Problem:** AmigaOS console.device sends cursor keys as CSI sequences (0x9B + letter), but mg's key bindings expect VT100 sequences (ESC + [ + letter). The METABIT handler in getkey() incorrectly decomposes 0x9B as Meta-ESC (ESC ESC) instead of ESC [.

**Files:**
- Modify: `ports/mg/ported/kbd.c:106` — add CSI check before METABIT handler

- [ ] **Step 1: Read current getkey() code**

Confirm the METABIT handler at kbd.c:106:
```c
if (use_metakey && (c & METABIT)) {
    pushedc = c & ~METABIT;
    pushed = TRUE;
    c = CCHR('[');
}
```

When 0x9B arrives: strips to 0x1B, pushes it, returns ESC. Next call returns ESC (the pushed 0x1B). Result: ESC ESC — wrong.

Correct decomposition: 0x9B (CSI) = ESC + `[` = 0x1B + 0x5B.

- [ ] **Step 2: Add CSI → ESC [ translation before METABIT**

In `ports/mg/ported/kbd.c`, replace the METABIT handler block (around line 106) with:

```c
#ifdef __AMIGA__
	/* amiport: AmigaOS console.device sends CSI (0x9B) for cursor keys,
	 * function keys, and other special keys. CSI is the 8-bit equivalent
	 * of ESC [ (0x1B 0x5B). Decompose it here so the VT100 escape
	 * sequence bindings in ansi.c (key_up = "\e[A", etc.) match correctly.
	 * Must check BEFORE the METABIT handler, which would incorrectly
	 * treat 0x9B as Meta-ESC (pushing 0x1B, returning ESC → ESC ESC). */
	if (c == 0x9B) {
		pushedc = '[';
		pushed = TRUE;
		c = CCHR('[');	/* ESC = 0x1B */
	} else
#endif
	if (use_metakey && (c & METABIT)) {
		pushedc = c & ~METABIT;
		pushed = TRUE;
		c = CCHR('[');
	}
```

This correctly decomposes 0x9B → ESC + `[`. The next ttgetc() returns the letter (A/B/C/D), giving ESC [ A/B/C/D — matching the key bindings.

- [ ] **Step 3: Build mg to verify compilation**

```bash
cd /Users/duncan/Developer/amiport && make build TARGET=ports/mg
```
Expected: Clean compilation, no errors.

- [ ] **Step 4: Run vamos smoke test**

```bash
cd /Users/duncan/Developer/amiport/ports/mg && make test VAMOS_STACK=256
```
Expected: mg launches and exits (basic smoke test — cursor keys can't be tested on vamos).

- [ ] **Step 5: Commit the cursor key fix**

```bash
git add ports/mg/ported/kbd.c
git commit -m "fix(mg): translate CSI 0x9B to ESC [ for cursor key support

AmigaOS console.device sends cursor keys as CSI sequences (0x9B + letter)
but mg's key bindings expect VT100 (ESC + [ + letter). The METABIT handler
incorrectly decomposed 0x9B as Meta-ESC. Add explicit CSI check before
METABIT to correctly split 0x9B into ESC + [."
```

---

## Task 2: Add Screen Read Trap to FS-UAE (Mode 150)

**Context:** The forked FS-UAE at `~/Developer/fs-uae-3.2.35/` already has consolehook modes 101/102. We add mode 150 to read ConUnit state from emulated Amiga memory and write it to a host-side file.

**Files:**
- Modify: `~/Developer/fs-uae-3.2.35/src/consolehook.cpp` — add `consolehook_read_screen()` function
- Modify: `~/Developer/fs-uae-3.2.35/src/filesys.cpp:8382` — add mode 150 dispatch
- Modify: `~/Developer/fs-uae-3.2.35/src/include/consolehook.h` — declare new function

**IMPORTANT:** Before writing this code, invoke `/amiga-api-lookup` to load ADCD docs. Verify ConUnit struct offsets against the ADCD header. Do NOT guess struct sizes — compute from the actual field order.

- [ ] **Step 1: Add consolehook_read_screen() to consolehook.cpp**

Add after the existing `consolehook_beginio()` function:

```cpp
/* amiport: ADR-025 — Read ConUnit screen state from emulated memory.
 * Called from mousehack_done mode 150.
 * Reads cursor position and screen dimensions from the ConUnit struct,
 * writes a JSON file to the ANSI log directory for host-side consumption.
 *
 * Args:
 *   conunit_addr — Amiga address of ConUnit (from ScreenRead binary)
 *                  If 0, uses first known unit from unit_logs[]
 *
 * ConUnit field offsets (from ADCD conunit.h):
 *   +62: cu_XCCP (WORD) — cursor column
 *   +64: cu_YCCP (WORD) — cursor row
 *   +42: cu_XMax (WORD) — max column
 *   +44: cu_YMax (WORD) — max row
 *   +38: cu_XCP  (WORD) — character position X
 *   +40: cu_YCP  (WORD) — character position Y
 */
uae_u32 consolehook_read_screen(uaecptr conunit_addr)
{
	if (!ansi_log_dir[0])
		return 0;

	/* If no ConUnit specified, use first known unit */
	if (conunit_addr == 0 && unit_log_count > 0)
		conunit_addr = unit_logs[0].unit_ptr;

	if (conunit_addr == 0) {
		write_log(_T("AMIPORT: read_screen: no ConUnit available\n"));
		return 0;
	}

	/* Read ConUnit fields from emulated memory */
	uae_s16 cu_xccp = (uae_s16)get_word(conunit_addr + 62);
	uae_s16 cu_yccp = (uae_s16)get_word(conunit_addr + 64);
	uae_s16 cu_xmax = (uae_s16)get_word(conunit_addr + 42);
	uae_s16 cu_ymax = (uae_s16)get_word(conunit_addr + 44);
	uae_s16 cu_xcp  = (uae_s16)get_word(conunit_addr + 38);
	uae_s16 cu_ycp  = (uae_s16)get_word(conunit_addr + 40);

	/* Write screen state to host-side file */
	char path[600];
	snprintf(path, sizeof(path), "%s/%08x.screen", ansi_log_dir, conunit_addr);
	FILE *fp = fopen(path, "w");
	if (!fp) {
		write_log(_T("AMIPORT: read_screen: can't open %s\n"), path);
		return 0;
	}

	fprintf(fp, "{\n");
	fprintf(fp, "  \"conunit\": \"0x%08x\",\n", conunit_addr);
	fprintf(fp, "  \"cursor_col\": %d,\n", cu_xccp);
	fprintf(fp, "  \"cursor_row\": %d,\n", cu_yccp);
	fprintf(fp, "  \"max_col\": %d,\n", cu_xmax);
	fprintf(fp, "  \"max_row\": %d,\n", cu_ymax);
	fprintf(fp, "  \"char_col\": %d,\n", cu_xcp);
	fprintf(fp, "  \"char_row\": %d\n", cu_ycp);
	fprintf(fp, "}\n");
	fclose(fp);

	write_log(_T("AMIPORT: read_screen %08x: cursor=(%d,%d) max=(%d,%d)\n"),
		conunit_addr, cu_xccp, cu_yccp, cu_xmax, cu_ymax);

	return 1;
}
```

- [ ] **Step 2: Declare in consolehook.h**

Add to `~/Developer/fs-uae-3.2.35/src/include/consolehook.h`:

```cpp
uae_u32 consolehook_read_screen(uaecptr conunit_addr);
```

- [ ] **Step 3: Add mode 150 dispatch in filesys.cpp**

In `mousehack_done()`, before the final `else` (around line 8382), add:

```cpp
	} else if (mode == 150) {
		/* amiport: ADR-025 — read ConUnit screen state.
		 * A1 = ConUnit address (0 = use first known unit) */
		return consolehook_read_screen(m68k_areg(regs, 1));
```

- [ ] **Step 4: Build FS-UAE**

```bash
cd ~/Developer/fs-uae-3.2.35 && make -j$(sysctl -n hw.ncpu)
```
Expected: Clean build. Copy binary to `~/Developer/fs-uae/fs-uae`.

- [ ] **Step 5: Commit FS-UAE changes**

```bash
cd ~/Developer/fs-uae-3.2.35
git add src/consolehook.cpp src/filesys.cpp src/include/consolehook.h
git commit -m "feat: add screen read trap mode 150 (ADR-025)

New mousehack_done mode 150 reads ConUnit cursor position and screen
dimensions from emulated Amiga memory. Writes JSON to AMIPORT_ANSI_LOG_DIR
for host-side visual verification. Supports both explicit ConUnit address
(from ScreenRead binary) and auto-detection from consolehook unit_logs."
```

---

## Task 3: Create ScreenRead Amiga Binary

**Purpose:** Amiga-side helper that finds the current console's ConUnit pointer and calls the FS-UAE trap mode 150 to dump screen state.

**Architecture:** Similar pattern to KeyInject. Gets ConUnit via ACTION_DISK_INFO packet to the console handler. Calls the trap by writing mode 150 + ConUnit address to a shared memory location and invoking the trap entry point.

**IMPORTANT:** Before writing this code:
1. Invoke `/amiga-api-lookup` to load ADCD docs for ACTION_DISK_INFO, DoPkt, FileHandle, InfoData
2. Invoke `/c89-reference` for compiler constraints
3. Verify struct FileHandle layout in ADCD

**Files:**
- Create: `toolchain/screenread/screenread.c`
- Create: `toolchain/screenread/Makefile`

- [ ] **Step 1: Research the trap calling convention**

The mousehack_done trap lives at `rtarea_base + 0xFF38`. From the Amiga side, it's called by JSR to that address. The rtarea_base is typically at 0x00F00000 in UAE configurations.

However, there's a simpler approach: use the **uaelib** interface. The uaelib trap (`uaelib_demux`) is accessible at a known address. But adding a new uaelib function is more invasive.

**Chosen approach:** Call the mousehack_done trap directly. The Amiga binary:
1. Gets ConUnit via ACTION_DISK_INFO
2. Puts mode 150 in D0, ConUnit in A1
3. JSR to the trap address (0x00F0FF38)

The trap address is `rtarea_base + 0xFF38`. In standard UAE config, rtarea_base = 0x00F00000. We can verify by checking for the UAE ROM signature.

- [ ] **Step 2: Write screenread.c**

```c
/* ScreenRead -- read console screen state via FS-UAE trap
 *
 * Triggers FS-UAE trap mode 150 to read ConUnit cursor position
 * and screen dimensions. Used by the automated visual test runner.
 *
 * Usage: ScreenRead
 *   Reads current console's ConUnit and triggers host-side dump.
 *   Output goes to AMIPORT_ANSI_LOG_DIR/<conunit>.screen on host.
 *
 * Requirements: FS-UAE with amiport consolehook (ADR-025 trap)
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

static const char *verstag = "$VER: ScreenRead 1.0 (24.03.2026)";

/* UAE trap address: rtarea_base (0x00F00000) + mousehack_done offset (0xFF38) */
#define UAE_TRAP_ADDR 0x00F0FF38UL

/* Get the ConUnit pointer for the current console window.
 * Uses ACTION_DISK_INFO packet to the console handler.
 * Returns ConUnit address or 0 on failure. */
static APTR get_conunit(void)
{
    struct MsgPort *con_handler;
    struct InfoData *id;
    struct IOStdReq *ioreq;
    struct FileHandle *fh;
    BPTR input_fh;
    APTR conunit = NULL;

    input_fh = Input();
    if (!input_fh)
        return NULL;

    /* Get the console handler port from the FileHandle */
    fh = (struct FileHandle *)BADDR(input_fh);
    con_handler = fh->fh_Type;
    if (!con_handler)
        return NULL;

    /* Allocate InfoData (must be longword-aligned, public memory) */
    id = (struct InfoData *)AllocMem(sizeof(struct InfoData),
                                      MEMF_PUBLIC | MEMF_CLEAR);
    if (!id)
        return NULL;

    /* Send ACTION_DISK_INFO to get console info.
     * id_InUse will contain the IOStdReq pointer for the console unit. */
    if (DoPkt1(con_handler, ACTION_DISK_INFO, MKBADDR(id))) {
        ioreq = (struct IOStdReq *)id->id_InUse;
        if (ioreq)
            conunit = (APTR)ioreq->io_Unit;
    }

    FreeMem(id, sizeof(struct InfoData));
    return conunit;
}

/* Call FS-UAE trap mode 150 with ConUnit pointer.
 * D0 = mode (150), A1 = ConUnit address.
 * Returns trap result in D0. */
static LONG call_screen_trap(APTR conunit)
{
    register LONG result __asm("d0");
    register LONG mode __asm("d0") = 150;
    register APTR unit __asm("a1") = conunit;

    __asm volatile (
        "jsr 0x00F0FF38"
        : "=r" (result)
        : "r" (mode), "r" (unit)
        : "d1", "a0", "a2", "a3", "a4", "a5", "a6", "memory"
    );

    return result;
}

int main(int argc, char **argv)
{
    APTR conunit;

    conunit = get_conunit();
    if (!conunit) {
        PutStr("ScreenRead: cannot find ConUnit\n");
        return 10;
    }

    if (!call_screen_trap(conunit)) {
        PutStr("ScreenRead: trap failed (not running in FS-UAE?)\n");
        return 10;
    }

    return 0;
}
```

**NOTE:** The inline asm for the trap call may need adjustment. The `jsr` to the rtarea trap address triggers FS-UAE's trap dispatcher which reads D0 for the mode. If this doesn't work with inline asm due to register allocation, the fallback is a small asm stub file.

- [ ] **Step 3: Write Makefile**

```makefile
# Build ScreenRead for AmigaOS
# Screen state reader for visual verification testing (ADR-025)

CC = ../scripts/m68k-amigaos-gcc
CFLAGS = -O2 -noixemul -m68000 -Wall
SCREENREAD_BIN = ScreenRead

all: $(SCREENREAD_BIN)

$(SCREENREAD_BIN): screenread.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(SCREENREAD_BIN)

.PHONY: all clean
```

- [ ] **Step 4: Build ScreenRead**

```bash
cd /Users/duncan/Developer/amiport/toolchain/screenread && make
```
Expected: Clean compilation, `ScreenRead` binary produced.

- [ ] **Step 5: Add build target to root Makefile**

Add alongside `build-keyinject`:
```makefile
build-screenread:  ## Build ScreenRead (screen state reader for visual tests)
	$(MAKE) -C toolchain/screenread
```

- [ ] **Step 6: Update install-emu to copy ScreenRead**

In the root Makefile's `install-emu` target, add ScreenRead alongside KeyInject:
```makefile
# Copy to FS-UAE work directory
cp toolchain/screenread/ScreenRead $(EMU_WORK_DIR)/
```

- [ ] **Step 7: Commit ScreenRead**

```bash
git add toolchain/screenread/ Makefile
git commit -m "feat: add ScreenRead binary for visual test screen capture (ADR-025)

Amiga-side helper that finds the console's ConUnit via ACTION_DISK_INFO
and calls FS-UAE trap mode 150 to dump cursor position and screen
dimensions to the host filesystem."
```

---

## Task 4: Update Test Infrastructure

**Files:**
- Modify: `scripts/test-fsemu.sh` — support SCREEN_READ directive in test cases
- Modify: `scripts/verify-screen.py` — read .screen JSON files for cursor assertions

- [ ] **Step 1: Add SCREEN_READ support to test-fsemu.sh**

In the test-fsemu.sh visual pass handler, after KeyInject execution and wait, add ScreenRead invocation. When a SCRAPE test contains `EXPECT_CURSOR` directives, the harness calls ScreenRead to dump ConUnit state before running verify-screen.py.

The ARexx test harness needs to call ScreenRead after the program has processed the injected keys. Add handling for `SCREEN_READ` as a KEYS token:

In the ARexx test runner, after the KEYS delay:
```rexx
/* After KeyInject, trigger screen read for SCRAPE tests */
ADDRESS COMMAND 'WORK:ScreenRead'
```

- [ ] **Step 2: Add EXPECT_CURSOR to verify-screen.py**

`EXPECT_CURSOR row,col` checks the .screen JSON file:

```python
def verify_cursor(screen_file, expected_row, expected_col):
    """Verify cursor position from FS-UAE trap screen dump."""
    import json
    with open(screen_file) as f:
        data = json.load(f)
    actual_row = data['cursor_row']
    actual_col = data['cursor_col']
    if actual_row != expected_row or actual_col != expected_col:
        return False, f"cursor at ({actual_row},{actual_col}), expected ({expected_row},{expected_col})"
    return True, f"cursor at ({actual_row},{actual_col})"
```

The verify-screen.py already handles EXPECT_AT (text at position via pyte). EXPECT_CURSOR is simpler — just reads the JSON.

- [ ] **Step 3: Wire screen files into test-fsemu.sh**

In test-fsemu.sh, when processing SCRAPE tests:
1. After KeyInject + wait, run ScreenRead in the emulated environment
2. Pass the .screen file path to verify-screen.py along with the ANSI log
3. verify-screen.py checks EXPECT_CURSOR directives against the .screen JSON

- [ ] **Step 4: Commit test infrastructure changes**

```bash
git add scripts/test-fsemu.sh scripts/verify-screen.py
git commit -m "feat: add EXPECT_CURSOR directive for trap-based cursor verification (ADR-025)

test-fsemu.sh calls ScreenRead after KeyInject in SCRAPE tests.
verify-screen.py reads the .screen JSON for EXPECT_CURSOR assertions.
Complements the pyte-based EXPECT_AT for text verification."
```

---

## Task 5: Write mg Visual Proving Test

**Purpose:** Prove the full pipeline works: inject DOWN arrow → mg moves cursor → trap reads cursor position → host-side assertion passes.

**Files:**
- Modify: `ports/mg/test-fsemu-visual-cases.txt` — add cursor movement test

- [ ] **Step 1: Write the visual test case**

Add to `ports/mg/test-fsemu-visual-cases.txt`:

```
ITEST: Visual: cursor moves down with arrow key
LAUNCH: WORK:mg -n WORK:test-scroll.txt
KEYS: WAIT2000,DOWN,WAIT1000
SCRAPE
SCREEN_READ
EXPECT_CURSOR 2,1
EXPECT_RC: 0
CLEANUP: CTRL_X,WAIT300,CTRL_C
```

This test:
1. Launches mg with a test file (cursor starts at row 1, col 1)
2. Waits for mg to initialize (2s)
3. Presses DOWN arrow
4. Waits for mg to process (1s)
5. Calls ScreenRead to capture cursor position via trap
6. Asserts cursor is at row 2, col 1
7. Quits mg with C-x C-c

- [ ] **Step 2: Ensure test input file exists**

Verify `ports/mg/test-scroll.txt` exists (used by existing tests). If not, create it with multi-line content for scrolling tests.

- [ ] **Step 3: Run the visual test**

```bash
cd /Users/duncan/Developer/amiport
make test-fsemu TARGET=ports/mg VISUAL=1
```
Expected: The cursor movement test passes — cursor at (2,1) after DOWN arrow.

- [ ] **Step 4: Commit the visual test**

```bash
git add ports/mg/test-fsemu-visual-cases.txt
git commit -m "test(mg): add cursor movement visual test proving ADR-025

Injects DOWN arrow via KeyInject, reads cursor position via ScreenRead
trap, asserts cursor moved to row 2. Proves the full ADR-025 pipeline:
CSI fix + trap handler + ScreenRead + EXPECT_CURSOR."
```

---

## Task 6: Write ADR-025 Document

**Files:**
- Create: `docs/adr/025-screen-read-trap.md`
- Modify: `README.md` — add ADR index entry
- Modify: `CLAUDE.md` — update ADR list

- [ ] **Step 1: Write the ADR**

Standard ADR format: Status, Context, Decision, Consequences. Reference ADR-024 as predecessor.

Key decisions to document:
- Trap mode 150 in mousehack_done (not uaelib) — same pattern as consolehook
- ConUnit read from emulated memory (not character map) — cu_XCCP/cu_YCCP
- JSON output format for host-side consumption
- ScreenRead binary uses ACTION_DISK_INFO to find ConUnit
- CSI 0x9B translation in mg as the proving fix
- EXPECT_CURSOR directive in test framework

- [ ] **Step 2: Update documentation index**

Add ADR-025 to README.md and CLAUDE.md ADR lists.

- [ ] **Step 3: Update known-pitfalls.md**

Add the CSI translation pitfall — this affects ALL console programs that use VT100 key bindings on AmigaOS.

- [ ] **Step 4: Update bug_mg_cursor_keys.md memory**

Mark as RESOLVED with the fix details.

- [ ] **Step 5: Commit documentation**

```bash
git add docs/adr/025-screen-read-trap.md README.md CLAUDE.md docs/references/crash-patterns.md
git commit -m "docs: ADR-025 screen read trap for interactive visual verification"
```

---

## Verification Checklist

Before considering this complete:

- [ ] mg cursor keys work on FS-UAE (manual check: launch mg, press arrow keys)
- [ ] ScreenRead binary builds and runs on FS-UAE
- [ ] .screen JSON file appears in AMIPORT_ANSI_LOG_DIR after ScreenRead runs
- [ ] verify-screen.py correctly reads EXPECT_CURSOR from .screen JSON
- [ ] Visual test for mg cursor movement passes: `make test-fsemu TARGET=ports/mg VISUAL=1`
- [ ] All existing mg functional tests still pass: `make test-fsemu TARGET=ports/mg`
- [ ] ADR-025 document written and indexed
- [ ] CSI translation pitfall added to known-pitfalls.md
