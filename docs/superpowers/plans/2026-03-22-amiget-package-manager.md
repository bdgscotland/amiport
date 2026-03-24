# amiget Package Manager Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first package manager for classic 68k AmigaOS — `amiget install grep` downloads, verifies, and installs packages from the live amiport API.

**Architecture:** Two new components: (1) `lib/http-shim/` — reusable HTTP/1.0 client library on bsdsocket-shim, (2) `ports/amiget/` — CLI binary with 6 commands (list, search, info, install, installed, help), JSON stream parser, SHA256 verification, and S:amiget.db tracking. All C89, static buffers, ≤35KB binary.

**Tech Stack:** C89 (bebbo-gcc), AmigaOS 3.x, bsdsocket.library via bsdsocket-shim, posix-shim, HTTP/1.0 over plain TCP.

**Design doc:** `~/.gstack/projects/bdgscotland-amiport/duncan-main-design-20260322-225928.md` (APPROVED)
**Eng review:** 9 decisions applied (archive validation, 30s timeout, atomic DB writes, etc.)
**Test plan:** `~/.gstack/projects/bdgscotland-amiport/duncan-main-eng-review-test-plan-20260322-234353.md`

---

## File Structure

### New: lib/http-shim/ (reusable HTTP/1.0 client)

| File | Responsibility |
|------|---------------|
| `lib/http-shim/include/amiport-net/http.h` | Public API: `amiport_http_get()`, `amiport_http_parse_url()` |
| `lib/http-shim/src/http.c` | HTTP GET implementation + URL parsing (no separate url.c — design doc directory listing is superseded by this plan) |
| `lib/http-shim/Makefile` | Build `libhttp-shim.a` |
| `tests/net/test_http.c` | Unit tests for URL parsing (runs on vamos — no network needed) |

### New: ports/amiget/ (the package manager CLI)

| File | Responsibility |
|------|---------------|
| `ports/amiget/ported/amiget.c` | Main entry point, command dispatch, list/search/info/install/installed/help |
| `ports/amiget/ported/json.c` | Manifest JSON stream parser with proper value skipping |
| `ports/amiget/ported/json.h` | Parser API + package struct definition |
| `ports/amiget/ported/sha256.c` | Minimal SHA256 (Brad Conte's public domain implementation) |
| `ports/amiget/ported/sha256.h` | SHA256 API |
| `ports/amiget/ported/db.c` | S:amiget.db read-modify-write with temp+rename |
| `ports/amiget/ported/db.h` | DB API |
| `ports/amiget/Makefile` | Build amiget, link against http-shim + bsdsocket-shim + posix-shim |
| `ports/amiget/PORT.md` | Porting log (this is original code, not a port) |
| `ports/amiget/amiget.readme` | Aminet readme |
| `ports/amiget/original/` | Empty dir (no upstream — original code) |
| `ports/amiget/test-fsemu-cases.txt` | FS-UAE test suite |

### Modified

| File | Change |
|------|--------|
| `Makefile` (project root) | Add `build-http-shim`, `test-http-shim` targets |
| `PORTS.md` | Add amiget entry |
| `README.md` | Add amiget to ports table |
| `CLAUDE.md` | Add lib/http-shim to codebase map, update agent table |
| `docs/architecture.md` | Add http-shim to pipeline diagram |

---

## Task 1: lib/http-shim — URL Parser + Header

**Files:**
- Create: `lib/http-shim/include/amiport-net/http.h`
- Create: `lib/http-shim/src/http.c` (URL parsing portion only)
- Create: `lib/http-shim/Makefile`
- Create: `tests/net/test_http.c`

- [ ] **Step 0: Create directory structure**

```bash
mkdir -p lib/http-shim/include/amiport-net lib/http-shim/src
```

- [ ] **Step 1: Create the http.h header with the public API**

Note: `amiport_http_get_mem()` was dropped per eng review #5 (file-only API — one code path).

```c
/* lib/http-shim/include/amiport-net/http.h */
#ifndef AMIPORT_NET_HTTP_H
#define AMIPORT_NET_HTTP_H

/* Fetch URL to a file. Returns 0 on success, -1 on error.
 * Sets *http_status to HTTP status code. Pass NULL for progress to suppress. */
int amiport_http_get(const char *url, const char *dest_path,
                     int *http_status,
                     void (*progress)(long received, long total));

/* Parse URL into components. Returns 0 on success, -1 on error.
 * Only http:// URLs are accepted. */
int amiport_http_parse_url(const char *url, char *host, int hostsize,
                           int *port, char *path, int pathsize);

#endif
```

- [ ] **Step 2: Write URL parsing tests**

Create `tests/net/test_http.c` with test cases for:
- Valid `http://host/path` → host="host", port=80, path="/path"
- URL with explicit port `http://host:8080/path` → port=8080
- URL with no path `http://host` → path="/"
- `https://` URL → returns -1
- Malformed URL (no `://`) → returns -1
- Empty host → returns -1
- Oversized host (>127 chars) → returns -1
- Oversized path (>255 chars) → returns -1

Use the `tests/net/test_framework.h` pattern from existing tests.

- [ ] **Step 3: Implement `amiport_http_parse_url()` in http.c**

Parse `http://host[:port]/path`. Static buffers. Return -1 for any non-http scheme.

- [ ] **Step 4: Create the Makefile for lib/http-shim**

Follow `lib/bsdsocket-shim/Makefile` pattern. Build `libhttp-shim.a`. Include paths: `-Iinclude -I../bsdsocket-shim/include -I../posix-shim/include`.

- [ ] **Step 5: Build and test URL parsing**

```bash
make -C lib/http-shim
cd tests/net && make test_http && vamos -s 64 ./test_http
```

Note: `tests/net/Makefile` uses bare `vamos` (not toolchain scripts). Ensure vamos is on PATH.

- [ ] **Step 6: Commit**

```bash
git add lib/http-shim/ tests/net/test_http.c tests/net/Makefile
git commit -m "feat: add lib/http-shim with URL parser and unit tests"
```

---

## Task 2: lib/http-shim — HTTP GET Implementation

**Files:**
- Modify: `lib/http-shim/src/http.c` (add HTTP GET)

- [ ] **Step 1: Implement `amiport_http_get()`**

The core HTTP/1.0 client. Implementation details:
- Static 2KB receive buffer
- `amiport_http_parse_url()` to decompose the URL
- `amiport_gethostbyname()` for DNS (from bsdsocket-shim)
- `amiport_socket()` + `amiport_connect()` to establish TCP connection
- Set `SO_RCVTIMEO`/`SO_SNDTIMEO` to 30 seconds via `amiport_setsockopt()`
- Send: `GET <path> HTTP/1.0\r\nHost: <host>\r\nConnection: close\r\nUser-Agent: amiget/1.0 (AmigaOS; 68k)\r\n\r\n`
- Parse response status line: `HTTP/1.x <status> <reason>`
- Parse headers: extract `Content-Length`, `Location` (for redirects), `Transfer-Encoding`
- If `Transfer-Encoding: chunked` → return -1 (not supported)
- If status 301/302 → follow `Location` (up to 3 redirects). If Location starts with `https://` → return -1
- Read body in 2KB chunks, write to dest_path file via `fopen()`/`fwrite()`/`fclose()`
- Call progress callback if non-NULL: `progress(bytes_received, content_length)`
- If Content-Length present, validate total bytes match
- atexit handler to close socket if exit during transfer
- `amiport_check_break()` in receive loop for Ctrl-C

- [ ] **Step 2: Build http-shim**

```bash
make -C lib/http-shim clean all
```

- [ ] **Step 3: Commit**

```bash
git add lib/http-shim/src/http.c
git commit -m "feat: implement HTTP/1.0 GET with redirects, timeouts, progress"
```

---

## Task 3: SHA256 Module

**Files:**
- Create: `ports/amiget/ported/sha256.h`
- Create: `ports/amiget/ported/sha256.c`

- [ ] **Step 1: Write SHA256 header**

```c
#ifndef AMIGET_SHA256_H
#define AMIGET_SHA256_H

/* Hash a file. Writes 64-char hex string to out (must be >=65 bytes).
 * Returns 0 on success, -1 on error. */
int sha256_file(const char *path, char *out);

/* Hash a memory buffer. */
void sha256_hash(const unsigned char *data, unsigned long len, char *out);

#endif
```

- [ ] **Step 2: Implement SHA256**

Use Brad Conte's public domain implementation (crypto-algorithms). ~300 lines. All static state, no malloc. Output as lowercase hex string.

- [ ] **Step 3: Write SHA256 test**

Test known vectors:
- Empty string → `e3b0c44298fc1c149afb...`
- "abc" → `ba7816bf8f01cfea...`
- "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" → known hash

- [ ] **Step 4: Build and test**

SHA256 vectors are tested as a standalone program compiled for vamos:

```bash
# Create a minimal test_sha256.c that calls sha256_hash() with known vectors
# Build: m68k-amigaos-gcc -O2 -noixemul -o test_sha256 test_sha256.c sha256.c
# Run: vamos -s 64 ./test_sha256
```

The test binary is temporary — delete after verification. SHA256 is also implicitly tested as part of Task 10's amiget smoke tests.

- [ ] **Step 5: Commit**

```bash
git add ports/amiget/ported/sha256.c ports/amiget/ported/sha256.h
git commit -m "feat: add minimal SHA256 for download integrity verification"
```

---

## Task 4: JSON Manifest Parser

**Files:**
- Create: `ports/amiget/ported/json.h`
- Create: `ports/amiget/ported/json.c`

- [ ] **Step 1: Define package struct and parser API in json.h**

```c
#ifndef AMIGET_JSON_H
#define AMIGET_JSON_H

#define AMIGET_MAX_PACKAGES 64  /* dynamic: 32 on <2MB, 64 on >=2MB */

struct amiget_package {
    char name[32];
    char version[16];
    char description[80];
    char category[32];
    char download[64];
    char sha256[65];
    long size;
    long stack;
    char requires[4][32];
    int  num_requires;
    /* Rich fields for info command */
    char source[64];
    char license[32];
    char porting_notes[256];
    char known_limitations[128];
    long downloads;
};

/* Parse manifest JSON from file. Returns number of packages parsed, or -1 on error. */
int amiget_parse_manifest(const char *path, struct amiget_package *pkgs, int max_pkgs);

/* Parse single-package JSON from file (for info command). Returns 0 on success. */
int amiget_parse_package(const char *path, struct amiget_package *pkg);

#endif
```

- [ ] **Step 2: Create a test manifest fixture**

Save a copy of the live manifest (or a representative subset) as a test fixture in the port directory for unit testing.

- [ ] **Step 3: Write the JSON parser**

Implement in `json.c`:
- Open file, read into static buffer (or stream if too large)
- Scan for `"packages":[`
- For each `{...}` object: match known keys, copy values into struct
- Proper value skipping for unknown fields (eng review #6):
  - String: scan to unescaped `"`
  - Array: count `[` depth
  - Object: count `{` depth
  - Number/bool/null: scan to `,` or `}`
- Handle escaped quotes, null values, missing fields
- Return error on truncated JSON

- [ ] **Step 4: Test parser against manifest fixture**

Verify all 13 packages parse correctly. Test edge cases: empty array, unknown fields, truncated file.

- [ ] **Step 5: Commit**

```bash
git add ports/amiget/ported/json.c ports/amiget/ported/json.h
git commit -m "feat: add JSON manifest parser with proper value skipping"
```

---

## Task 5: Installed Package DB

**Files:**
- Create: `ports/amiget/ported/db.h`
- Create: `ports/amiget/ported/db.c`

- [ ] **Step 1: Define DB API**

```c
#ifndef AMIGET_DB_H
#define AMIGET_DB_H

struct amiget_installed {
    char name[32];
    char version[16];
    char path[64];
};

/* Load installed packages. Returns count, or 0 if no DB. */
int amiget_db_load(struct amiget_installed *entries, int max_entries);

/* Add or update an entry. Uses temp+rename for crash safety. Returns 0 on success. */
int amiget_db_save(const char *name, const char *version, const char *path);

/* Check if a package is installed. Returns version string or NULL. */
const char *amiget_db_find(const char *name);

#endif
```

- [ ] **Step 2: Implement read-modify-write DB**

- `amiget_db_load()`: open `S:amiget.db`, read all lines, parse `name version path`
- `amiget_db_save()`: load existing DB, update/add entry, write to `S:amiget.db.tmp`, check `fclose()` return, `Rename()` over original (AmigaDOS Rename, not POSIX rename)
- `amiget_db_find()`: load and search by name
- Static buffer for up to 64 entries

- [ ] **Step 3: Commit**

```bash
git add ports/amiget/ported/db.c ports/amiget/ported/db.h
git commit -m "feat: add installed package DB with atomic writes"
```

---

## Task 6: amiget CLI — Scaffolding + Help + List

**Files:**
- Create: `ports/amiget/ported/amiget.c`
- Create: `ports/amiget/Makefile`
- Create: `ports/amiget/PORT.md`
- Create: `ports/amiget/original/` (empty dir)
- Create: `ports/amiget/amiget.readme`

- [ ] **Step 1: Create port directory structure**

```bash
mkdir -p ports/amiget/original ports/amiget/ported ports/amiget/.claude/agent-memory
```

- [ ] **Step 2: Write the Makefile**

```makefile
# Makefile for amiget (amiport package manager)
# DESCRIPTION is 34 chars (Aminet limit: 40)
include ../common.mk

TARGET      = amiget
VERSION     = 1.0
DESCRIPTION = Package manager for AmigaOS 3.x
AUTHOR      = Duncan Bowring
CATEGORY    = 4

SOURCES     = ported/amiget.c ported/json.c ported/sha256.c ported/db.c

# Network + HTTP libraries
CFLAGS  += -I../../lib/bsdsocket-shim/include -I../../lib/http-shim/include
LDFLAGS  = -L../../lib/http-shim -L../../lib/bsdsocket-shim -L../../lib/posix-shim \
           -lhttp-shim -lamiport-net -lamiport

# Explicit library dependencies for rebuild tracking
HTTP_LIB = ../../lib/http-shim/libhttp-shim.a
NET_LIB  = ../../lib/bsdsocket-shim/libamiport-net.a
SHIM_LIB = ../../lib/posix-shim/libamiport.a

$(TARGET): $(SOURCES) $(HTTP_LIB) $(NET_LIB) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

VAMOS_STACK = 256

.PHONY: all clean test

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Smoke tests (no network required)
	../../toolchain/scripts/vamos -C "" -s $(VAMOS_STACK) ./$(TARGET)
	../../toolchain/scripts/vamos -C "" -s $(VAMOS_STACK) ./$(TARGET) help
```

- [ ] **Step 3: Write amiget.c with command dispatch, help, and list**

The main file. Includes:
- `$VER` string and `__stack = 65536`
- `main()` parses argv[1] as subcommand
- `cmd_help()` prints usage
- `cmd_list()` fetches manifest (or reads cache), displays table with ANSI color
- `cmd_update()` force-refreshes the cache (kept for user convenience despite auto-refresh — costs ~5 lines)
- ANSI color helpers (check isatty)
- `atexit()` cleanup handler
- No args → print usage, exit 0
- Unknown command → error, exit 10
- Manifest fetch helper: try cache first, if not present or corrupt fetch from API and write cache

- [ ] **Step 4: Build and smoke test**

```bash
make -C lib/http-shim && make -C ports/amiget
cd ports/amiget && make test
```

- [ ] **Step 5: Commit**

```bash
git add ports/amiget/
git commit -m "feat: amiget CLI scaffold with help and list commands"
```

---

## Task 7: amiget CLI — search, info, installed Commands

**Files:**
- Modify: `ports/amiget/ported/amiget.c`

- [ ] **Step 1: Implement `cmd_search()`**

Case-insensitive substring match on name + description. Same table format as list, filtered. "No packages match '<term>'." if empty.

- [ ] **Step 2: Implement `cmd_info()`**

Fetch single package from API (`/api/v1/packages.php?name=<name>`), parse with `amiget_parse_package()`. Display all rich fields. Fall back to cache if network unavailable.

- [ ] **Step 3: Implement `cmd_installed()`**

Read `S:amiget.db` via `amiget_db_load()`. Display table. "No packages installed." if empty.

- [ ] **Step 4: Build and test**

```bash
make -C ports/amiget clean all && cd ports/amiget && make test
```

- [ ] **Step 5: Commit**

```bash
git add ports/amiget/ported/amiget.c
git commit -m "feat: add search, info, and installed commands"
```

---

## Task 8: amiget CLI — install Command

**Files:**
- Modify: `ports/amiget/ported/amiget.c`

- [ ] **Step 1: Implement `cmd_install()`**

The full 14-step install flow from the design doc:
1. Load manifest (cache or fetch)
2. Find package by name
3. Check if already installed (DB)
4. Check `C:lha` exists
5. Check `requires` via `OpenLibrary()` (warn only)
6. Download to `T:<name>-<version>.lha` with progress
7. SHA256 verify
8. Archive validation via `lha l` — reject non-C/ entries
9. Extract via `SystemTags()`
10. Verify binary at `C:<name>`
11. Update DB
12. Cleanup temp file
13. Print success
14. Exit 0

Each error path prints a specific message and exits with the correct code.

- [ ] **Step 2: Build and smoke test**

```bash
make -C ports/amiget clean all && cd ports/amiget && make test
```

Note: install command can't be tested on vamos (no network). FS-UAE testing is deferred.

- [ ] **Step 3: Commit**

```bash
git add ports/amiget/ported/amiget.c
git commit -m "feat: implement install command with SHA256, archive validation, DB tracking"
```

---

## Task 9: PORT.md, Readme, and Documentation

**Files:**
- Create: `ports/amiget/PORT.md`
- Create: `ports/amiget/amiget.readme`
- Modify: `PORTS.md`
- Modify: `README.md`
- Modify: `CLAUDE.md`
- Modify: `docs/architecture.md`

- [ ] **Step 1: Write PORT.md**

Document that this is original code (not a port). List all design decisions, dependencies, and the eng review outcomes.

- [ ] **Step 2: Write amiget.readme using Aminet template**

Follow `ports/templates/readme.template`. DESCRIPTION must be ≤40 chars ASCII.

- [ ] **Step 3: Update PORTS.md**

Add amiget to the catalog table: name=amiget, version=1.0, description="Package manager for AmigaOS 3.x", category=Network, source=Original, status=Built & tested.

- [ ] **Step 4: Update README.md**

Add amiget to the ports summary table.

- [ ] **Step 5: Update CLAUDE.md**

Add `lib/http-shim/` to the codebase map. Note amiget as first Category 4 port.

- [ ] **Step 6: Update docs/architecture.md**

Add http-shim to the component diagram.

- [ ] **Step 6b: Update docs/porting-guide.md**

Mention http-shim as available for Category 4 (network) ports.

- [ ] **Step 7: Update root Makefile**

Add `build-http-shim` and `test-http-shim` targets.

- [ ] **Step 8: Commit**

```bash
git add PORTS.md README.md CLAUDE.md docs/architecture.md Makefile ports/amiget/PORT.md ports/amiget/amiget.readme
git commit -m "docs: add amiget to all project documentation"
```

---

## Task 10: vamos Smoke Tests + Build Verification

**Files:**
- Modify: `ports/amiget/Makefile` (expand test target)

- [ ] **Step 1: Expand vamos test suite**

Add smoke tests to Makefile:
- No args → prints usage, exit 0
- `help` → prints usage, exit 0
- Unknown command → exit 10
- `search` with no arg → exit 10
- `install` with no arg → exit 10
- Verify `$VER` string present in binary

- [ ] **Step 2: Run full test suite**

```bash
make -C ports/amiget clean all test
```

- [ ] **Step 3: Check binary size**

```bash
ls -la ports/amiget/amiget  # Target: ≤35KB
```

- [ ] **Step 4: Run project-wide checks**

```bash
make check-docs && make check-port-metadata
```

- [ ] **Step 5: Commit**

```bash
git add ports/amiget/Makefile
git commit -m "test: add vamos smoke tests for amiget CLI"
```

---

---

## Task 11: FS-UAE Test Suite Generation

**Files:**
- Create: `ports/amiget/test-fsemu-cases.txt`

- [ ] **Step 1: Dispatch test-designer agent**

Dispatch the `test-designer` agent (per port workflow rules) to generate `test-fsemu-cases.txt` from the amiget source. The agent analyzes command flags, exit codes, and error paths.

Note: amiget is a Category 4 (network) port. FS-UAE must have bsdsocket.library enabled for network tests to work. Smoke tests (help, unknown command, missing args) can run without network.

- [ ] **Step 2: Commit**

```bash
git add ports/amiget/test-fsemu-cases.txt
git commit -m "test: add FS-UAE test suite for amiget"
```

---

## Notes for Implementation

### Key constraints (read before writing any code)
- **C89 only** — no `//` comments, no for-init declarations, no mixed declarations, no `inline`
- **No malloc for I/O buffers** — use static arrays
- **All exit paths go through atexit()** — register cleanup early
- **`fprintf(stderr, ...)` for errors** — stdout is for data output
- **LONG is 32-bit** — use `%ld` format specifier
- **No `fclose(stdin)`** — see known-pitfalls.md
- **Invoke `/c89-reference` skill** when writing code
- **Invoke `/amiga-api-lookup` skill** when using AmigaOS APIs (OpenLibrary, SystemTags, etc.)

### Existing code to reference
- `lib/bsdsocket-shim/src/socket.c` — socket lifecycle pattern (auto-init, atexit cleanup)
- `lib/bsdsocket-shim/src/resolve.c` — DNS resolution wrapper
- `ports/grep/Makefile` — port Makefile pattern with multiple source files
- `ports/templates/readme.template` — Aminet readme format
- `tests/net/test_inet.c` — network test pattern
