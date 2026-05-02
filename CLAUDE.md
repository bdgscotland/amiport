# amiport — Claude Code Project Instructions

## What This Project Is

amiport is an AI-powered toolkit for porting POSIX C and C++ software to the Commodore Amiga -- CLI tools, network programs, console UIs, and SDL2 games. Claude is the primary porting agent -- this project provides the skills, agents, reference docs, shim/compatibility libraries, bundled algorithm libraries (crypto, compression, regex, fonts, soft-float), and a C89 + C++17 cross-compilation toolchain that enable automated porting. Built artifacts are distributed via Aminet and amiport.platesteel.net.

## Architecture

The porting pipeline has 4 stages, each backed by a Claude skill:

1. **Analyze** (`/analyze-source`) — Scan source for portability issues
2. **Transform** (`/transform-source`) — Replace POSIX calls with Amiga equivalents
3. **Build** (`/build-amiga`) — Cross-compile with bebbo-gcc or VBCC
4. **Test** (`/test-amiga`) — Run in vamos emulator, verify output

**Orchestration modes:**
- `/port-project` — Run the full pipeline end-to-end (single port)
- `/batch-port-parallel` — Dispatch N ports simultaneously in isolated worktrees

## Codebase Map

- `.claude/skills/` — Skill definitions for each pipeline stage
- `.claude/agents/` — 20 agent definitions (see agent table below for full list)
- `lib/posix-shim/` — Tier 1: Direct POSIX-to-AmigaOS wrappers (`amiport_*` functions)
- `lib/posix-emu/` — Tier 2: Approximate POSIX emulation with documented caveats (`amiport_emu_*` functions)
- `lib/posix-shim/include/amiport/compat.h` — Platform compatibility fixes for 68k quirks (alignment, byte order, alloca). Not a function library — a header with macros for issues that break correct C on 68k. Includes `AMIPORT_ALIGN()` (crash-patterns #15, #16) and `alloca` → `__builtin_alloca` shim.
- `lib/posix-shim/include/amiport/thread_stubs.h` — C++ threading stubs for bebbo-gcc 13.3 (`--enable-threads=no`). Provides no-op `std::mutex`, `std::recursive_mutex`, `std::condition_variable`. Use via `-include amiport/thread_stubs.h` for C++ ports with `-DNO_THREADS`. Safe on AmigaOS (no preemptive multitasking).
- `lib/console-shim/` — Minimal ncurses + termcap API mapped to Amiga console.device ANSI escapes (ADR-009). Includes termcap (tgetent/tgetstr/tgoto/tparm) for programs like less, and curses (initscr/getch/addch) for ncurses programs.
- `lib/posix-shim/include/amiport/termios.h` — Minimal termios shim mapping tcgetattr/tcsetattr to AmigaOS SetMode() for raw/cooked console mode. Used by terminal programs (less, nano, vim).
- `lib/bsdsocket-shim/` — BSD socket API via bsdsocket.library with auto lifecycle (ADR-010). Includes getaddrinfo/freeaddrinfo (wraps gethostbyname), inet_ntop/inet_pton (pure C, IPv4), fcntl non-blocking sockets (IoctlSocket FIONBIO). Extended for PDR-013 Dropbear SSH.
- `lib/http-shim/` — Reusable HTTP/1.0 GET client library on bsdsocket-shim. Used by amiport CLI. Handles redirects, Content-Length validation, progress callbacks, 30s socket timeout.
- `lib/oniguruma/` — Oniguruma 6.9.9 regex engine (ASCII-only build, 156 KB). Perl-compatible regex with named captures. Used by jq for test/match/sub/gsub. Unicode data tables replaced with stubs to save 312 KB.
- `lib/zlib/` — zlib 1.3.1 DEFLATE/gzip/zlib compression library (~90 KB). Pure C89, no amiport shim dependencies, uses libnix fd calls directly for gz* file I/O. Built `-O0` default with per-file `-O1` on hot-path files (`inffast.c`, `adler32.c`, `crc32.c`). `-DNO_DIVIDE` eliminates software divides in adler32. Required by libgit2/amigit (PDR-010) and available for any future compression-dependent ports.
- `lib/libgit2/` — libgit2 1.8.5 embedded git library (~1.44 MB). Dual-flavor build: `libgit2.a` (-m68000, default, used by every 68000-compatible port and by the Stage 5 test suite) and `libgit2-020.a` (-m68020, used by `ports/amigit/`). Clone/fetch/remote/push + smart-HTTP transport framing (clone.c, fetch.c, remote.c, transport.c, transports/smart.c + smart_pkt.c + smart_protocol.c) are PRESENT as of PDR-012 Phase 1. Still pruned: `transports/http.c`, `transports/git.c`, `transports/ssh_*.c`, `transports/local.c`, entire `streams/` subtree, threads, mmap, failalloc, win32. `transport_stubs.c` provides link-time stubs for the static dispatch table (git_smart_subtransport_http/_git/_ssh and git_transport_local return GIT_ERROR); amigit registers its own smart-HTTP subtransport via `git_transport_register` before any remote lookup so its backend preempts the stubs at runtime. Depends on `lib/zlib/` and `lib/posix-shim/`. Force-include `src/util/amigaos_compat.h` retargets `pread`/`pwrite`/`realpath`/`symlink`/`getpwuid_r`/`utimes`/`futimes` to the amiport shim; all other file I/O uses libnix native. Built `-O0` default with per-file `-O1 -fno-strict-aliasing` on 9 audited hot files (SHA1DC, xdiff core, wildmatch). `-DGIT_LEGACY_HASH` avoids a latent bus error in MurmurHash3 (68k alignment). Required by the `ports/amigit/` CLI (PDR-010 + PDR-012). Stage 5 tests at `tests/libgit2/` pass 79/79 on vamos with `-DAMIPORT_VAMOS_LIMITED` gating 6 directory-enumeration tests that need FS-UAE. Audit reports: `lib/libgit2/PATCHES.md`, `lib/libgit2/MEMORY-AUDIT.md`, `lib/libgit2/PERF-REPORT.md`.
- `lib/libtommath/` — LibTomMath 1.3.0 big integer arithmetic library (~90 KB). Pure C, zero OS dependencies. Built `-O0 -m68020 -std=gnu99` default with per-file `-O1` on 9 audited hot-path files (multiply, square, exptmod, Montgomery reduce). `-DMP_NO_FILE -DMP_LOW_MEM -DMP_FIXED_CUTOFFS -DMP_NO_DEV_URANDOM`. Required by LibTomCrypt and Dropbear SSH (PDR-013). Stage 5 tests at `tests/libtommath/` pass 25/25 on vamos with `-C 68020`.
- `lib/libtomcrypt/` — LibTomCrypt 1.18.2 cryptographic primitives library (~259 KB). Stripped build via `LTC_NOTHING` + selective enables: AES-CTR, ChaCha20-Poly1305, SHA-1/256/384/512, HMAC, RSA, ECDSA, DH, Fortuna PRNG, base64. Built `-O0 -m68020 -std=gnu99 -DLTC_NO_FILE -DLTM_DESC -DLTC_CLEAN_STACK`. 68k big-endian fast-path enabled via `tomcrypt_cfg.h` patch. Per-file `-O1` on 6 audited hot-path files (AES, ChaCha20, SHA-256, Poly1305, CTR, Fortuna). Depends on `lib/libtommath/`. Required by Dropbear SSH (PDR-013). Stage 5 tests at `tests/libtomcrypt/` pass 15/15 on vamos with `-C 68020`.
- `lib/amissl-sdk/` — AmiSSL SDK headers and stub libraries for optional HTTPS support (used by wget). See known-pitfalls re: libamisslauto.a hard dependency.
- `lib/softfloat/` — Pure-integer IEEE 754 soft-float helpers + Sun fdlibm software libm (~31 KB). Built `-m68020 -O1 -noixemul`. Provides `__divsf3`/`__mulsf3`/`__addsf3`/`__subsf3` and double-precision equivalents that override libnix's versions (which crash on FS-UAE because they delegate to ROM `mathieeesingbas.library` / `mathieeedoubbas.library`), AND software `sin`/`cos`/`tan`/`atan`/`atan2`/`sqrt`/`exp`/`log`/`log10`/`pow`/`fmod`/`fabs`/`floor`/`scalbn` + `sinf`/`cosf`/etc. wrappers. Source: promoted from libSDL2-amigaos3's `src/stdlib/SDL_os3*.c` + `src/libm/`. Link with `-Llib/softfloat -lsoftfloat` BEFORE `-lm`. Required by any C++ port that uses float arithmetic OR `std::ostream<<float` OR locale-aware number formatting on FS-UAE. Captured to amiga-kb as critical-severity pitfall. Isolation reproducer: `ports/softfloat-test/softfloat-test.cpp` -- 10-phase bisecting C++ test (single + double soft-float helpers, fdlibm transcendentals, printf %f/%g, ostringstream insertion). Build with `bash ports/softfloat-test/build.sh`, run with `fs-uae ports/softfloat-test/softfloat-test.fs-uae` (boots from `build/system-softfloat/` whose User-Startup launches the test and writes results to `WORK:softfloat-test/softfloat-test.log`). Phases 1-9 PASS on FS-UAE A3000/68030/68882 (lib/softfloat is sound); phase 10b5c crashes on `oss << (short)42` -- exercises the bebbo-gcc 13.3 std::ostream<<(int/short) Guru #80000008 pitfall (separate libstdc++ ABI bug, see known-pitfalls.md); phase 10b5c-10b5e GATED OUT by `#if ENABLE_OSTREAM_INT_REPRO` so the binary completes cleanly by default. Phase 11 (added 2026-04-17) tests fmt::format using OpenTTD's vendored fmt 7.x and PROVES fmt is INDEPENDENT of the libstdc++ ostream defect -- `fmt::format("{}", 42)`, `fmt::format(FMT_STRING("{}"), 42)`, and `fmt::format("{:.1f}", 3.14)` all PASS. Critical implication: OpenTTD's 604 `Debug()` calls all route through fmt::format(FMT_STRING(...)) and are SAFE; OpenTTD's post-CheckMD5 #80000003 alignment crash is genuinely unrelated to the libstdc++ defect. Note: `ports/softfloat-test/` is a DEBUG ISOLATION TEST, not a real port -- no Makefile, no PORT.md, no .readme; check-port-metadata.sh skips it because no Makefile is present.
- `lib/glyph-cache/` — Generic LRU glyph cache for AmigaOS text-rendering ports (~8 KB). Pure C99, no AmigaOS deps. Caches 8-bit alpha bitmaps keyed by `(face_id, codepoint, px_size, hint_flags)`. Bump-pointer arena with 4-byte alignment (avoids 68k offsetof=2 trap, crash-patterns #15). Hash table (512 slots, linear probe) + LRU doubly-linked list. v1 eviction is whole-arena flush when full -- simple, trades thrashing for tractability. Built `-O0 -m68000` (per crash-patterns #16 default-O0-for-bundled-libs rule). 6/6 tests pass on vamos. Reusable by any text-rendering port; built by NetSurf Vampire Phase 1 (PDR-XXX) as the cache layer between FreeType rasterization and AMMX glyph compositor. Top-level: `make build-glyph-cache`, `make test-glyph-cache`.
- `lib/libwapcaplet/` — NetSurf string interning library (~3 KB). Upstream `netsurf-browser/libwapcaplet` v0.4.3 @ commit `c7c128d`, MIT-licensed. Single TU 290 LOC, pure C99 with no POSIX surface beyond malloc/free/memcpy/memset/strncmp/assert (all libnix Tier 1). Reference-counted strings with FNV-1a hash (4091 buckets), lazy caseless "insensitive" twins, optional `lwc_iterate_strings()`-driven global-context cleanup. **Built `-O1 -m68040 -m68881` -- DEVIATION from project's standard `-m68000` library convention.** First lib of the NetSurf-Vampire Phase 1 dep stack (Phase D-prime, see `docs/superpowers/plans/2026-05-02-netsurf-vampire-phase-d-prime-dep-stack.md`); the entire dep stack matches the `ports/netsurf/` consumer ABI exactly to avoid mixed-CPU complexity. The `-O1` whole-TU promotion was audited 2026-05-02 against crash-patterns #16 (no struct returns >8 bytes, no float math, no alignment traps; clean for bebbo-gcc 13.3 codegen on this single TU). 36/36 tests pass on vamos `-C 68040`, including `self_insensitive_destroy_path` which empirically refutes a Stage-6 memory-checker false-positive on the destroy gate at libwapcaplet.c:196 (the existing `if (str->insensitive != NULL && str->refcnt == 0)` correctly short-circuits the recursive unref when entered via the macro's "refcnt==1 && insensitive==self" branch). **Consumer responsibility:** call `lwc_iterate_strings(NULL, NULL)` before program exit to free the global ~16 KB hash-table context plus residual strings -- AmigaOS `-noixemul` does NOT reclaim process memory on exit. Required by future Phase D-prime libs `lib/libcss/`, `lib/libdom/`, `lib/libhubbub/`, `lib/libsvgtiny/`, `lib/libnsutils/`, `lib/libnspsl/` (none shipped yet as of 2026-05-02). Top-level: `make build-libwapcaplet`, `make test-libwapcaplet`.
- `lib/libhubbub/` — NetSurf HTML5 tokeniser + tree builder (~312 KB). Upstream `netsurf-browser/libhubbub` v0.3.8 @ commit `6651b8c`, MIT-licensed. 30 .c files / ~8K LOC + 11647-line generated `entities.inc` (HTML5 named-entity bsearch table) + 705-line gperf `autogenerated-element-type.c` (perfect-hash element-name lookup). Both pre-generated files committed; regenerated via `perl build/make-entities.pl` and `gperf src/treebuilder/element-type.gperf` if upstream entity / element lists ever refresh. Subsystems: `charset/detect.c` (BOM + meta heuristics), `parser.c` (top-level glue), `tokeniser/tokeniser.c` (~3450 LOC, 70+ states, switch-per-state), `treebuilder/` (21 mode-specific files for the HTML5 tree-construction algorithm), `utils/`. Built `-O0 -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE -std=c99` -- same NetSurf-Vampire dep stack convention as `lib/libwapcaplet/` and `lib/libparserutils/`. **Depends on lib/libparserutils** (link order: `-lhubbub -lparserutils`). Memory-checker APPROVED -- fully reentrant (no globals), all 12 malloc/free pairs proper, 7 realloc patterns safe, no double-free / UAF; the 50-iteration parser-lifecycle stress test in tests verifies clean repeat create/destroy. Source-analyzer CLEAN (all 12 amiport concerns pass). 20/20 tests pass on vamos `-C 68040 -s 4096 -m 8192`. **CRITICAL vamos resource sizing:** test binaries linking libhubbub need `__stack >= 524288` AND `__MEMORY_STEP >= 524288` -- the 256 KB defaults that work for libwapcaplet/libparserutils trigger an Illegal Instruction crash inside libnix's stdio init at PC=0x01e16c (libnix needs more startup-time allocation when the binary's globals area grows past a threshold). **Tree-handler callback contract:** all `hubbub_string` data passed to consumer callbacks (tag names, attribute values, text content) is library-owned and only valid for the duration of the callback. Consumers must copy via `lwc_intern_string()` (the canonical NetSurf pattern) to retain. Required by future `lib/libdom/` (Phase D-prime Wave 2) and ultimately `ports/netsurf/`. Top-level: `make build-libhubbub`, `make test-libhubbub`.
- `lib/libparserutils/` — NetSurf parser primitives library (~78 KB). Upstream `netsurf-browser/libparserutils` v0.2.x @ commit `6b0cbf0`, MIT-licensed. 15 .c files / ~3.5K LOC + 1142-line generated `aliases.inc` (committed, not regenerated at build). Built `-O1 -fno-strict-aliasing -m68040 -m68881 -DWITHOUT_ICONV_FILTER -DNDEBUG -D_DEFAULT_SOURCE -std=c99` -- same NetSurf-Vampire dep stack convention as `lib/libwapcaplet/`. Provides built-in charset codecs (UTF-8, UTF-16 BOM-driven, ISO-8859-1..16, Windows-125x, US-ASCII, ext8 vendor variants), IANA MIB-enum alias resolution (~700 entries via `bsearch`), UTF-8/16 codepoint helpers, input stream with mid-stream `<meta charset>` switching, and buffer/stack/vector container primitives. **Critical define `-DWITHOUT_ICONV_FILTER`** switches `src/input/filter.c` from iconv (which AmigaOS lacks) to libparserutils' own complete codec subsystem -- no iconv shim needed. Whole-archive `-O1` promoted 2026-05-02 after perf-optimizer audit cleared all 15 TUs against crash-patterns #16 (zero struct-by-value returns >8 bytes, zero float math). Memory-checker APPROVED — clean malloc/free pairing across all subsystems, safe realloc patterns, no double-free / UAF. 57/57 tests pass on vamos `-C 68040`. **Two upstream behaviour quirks documented in lib/libparserutils/README.md:** (1) the codec subsystem only handles "UTF-16" (BOM-driven) -- explicit "UTF-16BE"/"UTF-16LE" need iconv (gone) or extra handler dispatch entries; (2) `utf16_from_ucs4` supplementary-plane encoding uses a non-standard formula that doesn't round-trip cleanly. Required by future Phase D-prime libs `lib/libcss/`, `lib/libdom/`, `lib/libhubbub/`, `lib/libsvgtiny/`. Top-level: `make build-libparserutils`, `make test-libparserutils`.
- `lib/libnsbmp/` — NetSurf BMP/ICO image decoder (~7 KB). Upstream `netsurf-browser/libnsbmp` v0.1.7 @ commit `ea063c9`, MIT-licensed. Single TU 1388 LOC with caller-supplied bitmap-allocation callbacks. Supports BMP RGB 1/4/8/16/24/32 bpp + RLE4 + RLE8 + bitfield encodings, top-down and bottom-up scan orders, ICO collections with mask alpha. Pure C99 with libnix Tier 1 stdlib only (assert/stdbool/stddef/stdint/stdio/stdlib/string). Built `-O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -std=c99` -- single TU is trivially -O1-safe per crash-patterns #16 (zero struct returns >8 bytes, zero soft-float pulls verified via nm). `-fno-strict-aliasing` is required because the decoder uses type-punning for endian-swapped pixel reads. **Standalone -- no NetSurf-internal deps.** Memory-checker APPROVED with one CAVEAT: `ico_header_parse` linked-list loop has no error-cleanup path on partial failure (`bmp_info_header_parse` mid-loop fail leaks the `ico_image *first` chain). Real-world risk is low (ICOs typically 1-6 sub-images, malloc rarely fails for ~64-byte allocs). Documented in lib/libnsbmp/README.md; deferred for upstream patch. 18/18 tests pass on vamos `-C 68040 -s 1024 -m 4096`. **Mandatory consumer cleanup:** `bmp_finalise(bmp)` for every successful create + `ico_finalise(ico)` for every ICO at exit. Required by future `ports/netsurf/` for inline `<img>` and `<link rel="icon">` decoding. Top-level: `make build-libnsbmp`, `make test-libnsbmp`.
- `lib/libnsgif/` — NetSurf animated GIF image decoder (~10 KB). Upstream `netsurf-browser/libnsgif` v1.0.x @ commit `5d5d750`, MIT-licensed. 2 TUs / 2689 LOC: gif.c (top-level decoder ~2076 LOC) + lzw.c (LZW decompressor ~613 LOC). Caller-supplied bitmap callbacks, GIF87a + GIF89a support, animation looping, 8 different output pixel formats (R8G8B8A8/B8G8R8A8/A8R8G8B8/A8B8G8R8/RGBA8888/BGRA8888/ARGB8888/ABGR8888), progressive (chunked) data feed for network-fetched GIFs. Pure C99 with libnix Tier 1 stdlib only (assert/inttypes/stdbool/stdint/stdlib/string). Built `-O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -std=c99` -- both TUs -O1-safe per crash-patterns #16 (one struct-by-value return `nsgif_colour_layout` at 4 bytes, well under 8-byte threshold; zero soft-float pulls verified via nm). **Standalone -- no NetSurf-internal deps.** Memory-checker APPROVED with no findings (12 malloc/free pairs all balanced, both realloc patterns use safe intermediate-pointer idiom -- the libdom realloc bug is NOT present here, bitmap callback discipline correct, LZW context lifecycle clean, frame array growth correctly initialises new entries, all error paths flow through `nsgif_destroy` cleanup). 16/16 tests pass on vamos `-C 68040 -s 1024 -m 4096`. **Mandatory consumer cleanup:** `nsgif_destroy(gif)` for every successful create at exit. Required by future `ports/netsurf/` for inline `<img>` and animated banner decoding. Top-level: `make build-libnsgif`, `make test-libnsgif`.
- `lib/libcss/` — NetSurf libcss CSS Cascading Style Sheets implementation (~535 KB). Upstream `netsurf-browser/libcss` v0.9.x @ commit `104d87f`, MIT-licensed. 302 .c files (185 hand-written + 119 autogenerated per-property parsers from `properties.gen` via `css_property_parser_gen.c` build-tool, pre-generated and committed). Subsystems: `src/lex/lex.c` (CSS tokeniser), `src/parse/` (parser + per-property parsers ~157 .c), `src/select/` (selector matcher + cascade + arena + per-property dispatchers ~100 .c), `src/charset/detect.c` (@charset + BOM), `src/utils/`, `src/stylesheet.c`. **CRITICAL design fact:** uses 22:10 fixed-point integer math (`css_fixed = int32_t` per `include/libcss/fpmath.h`), NOT floating-point -- zero soft-float pulls verified via `m68k-amigaos-nm` (no `__divsf3`/`__floatunsisf`/etc.). The `FLTTOFIX(0.9)` style macros are compile-time constants and fold to integer literals. Whole-archive `-O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE -std=c99`, audited 2026-05-02 -- all 302 TUs are -O1-safe per crash-patterns #16 (5 benign `___divdi3` references for 64-bit overflow-prevent in fixed-point evaluator are libgcc helpers, NOT crash-risk). **Two source files excluded from archive:** (1) `src/parse/properties/css_property_parser_gen.c` (build-time tool with main; pre-generates the 119 autogenerated_*.c files); (2) `src/select/format_list_style.c` -- 67 lines of UTF-8 (Georgian/Armenian/Greek alphabet tables for CSS list-style-type counter formatting); bebbo-gcc preprocessor silently corrupts code surrounding multi-byte UTF-8 chars. Functional consequence: exotic counter styles (`georgian`, `armenian`, `hiragana`, etc.) unsupported; common styles (decimal, disc, circle, square, lower-alpha, upper-alpha, lower-roman, upper-roman) work fine. **Depends on lib/libwapcaplet + lib/libparserutils** (link order: `-lcss -lwapcaplet -lparserutils`). Memory-checker APPROVED with no findings -- 7 stylesheet_create error paths all have proper cleanup, arena allocator uses safe realloc-via-intermediate-pointer pattern, no static mutable globals (no equivalent of libdom's `dom_namespaces[]`). Source-analyzer CAVEATS (the format_list_style.c exclusion above; everything else CLEAN). 38/38 tests pass on vamos `-C 68040 -s 4096 -m 8192`. **Vamos resource sizing:** test binaries linking the libcss dep stack need `__stack >= 1048576` AND `__MEMORY_STEP >= 1048576` (same as libdom-class -- shared with the existing pitfall). **Mandatory consumer cleanup:** `css_stylesheet_destroy(sheet)` for every stylesheet at exit; `css_select_ctx_destroy(ctx)` for every select context; `lwc_iterate_strings(NULL, NULL)` (libwapcaplet-shared cleanup hook covers libcss's interned strings). Required by future `lib/libsvgtiny/` and ultimately `ports/netsurf/`. Top-level: `make build-libcss`, `make test-libcss`.
- `lib/libdom/` — NetSurf libdom W3C DOM Level 3 implementation (~272 KB). Upstream `netsurf-browser/libdom` v0.4.2 @ commit `f69781e`, MIT-licensed. 95 .c files / ~36K LOC across 5 subsystems: `src/core/` (18 files, DOM core -- Node/Element/Document/Attr/Text/etc.), `src/utils/` (5 files, hashtable + namespace + walk + validate), `src/events/` (14 files, DOM Level 3 Events), `src/html/` (57 files, HTMLElement subclasses), `src/bindings/hubbub/parser.c` (the libhubbub-token -> DOM tree binding -- the canonical NetSurf entry point). We SKIP `bindings/xml/` (depends on expat/libxml -- not shipped). Whole-archive `-O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG -D_DEFAULT_SOURCE -D_BSD_SOURCE -std=c99`, audited 2026-05-02 -- all 95 TUs are -O1-safe (zero struct-by-value returns >8 bytes, zero soft-float pulls verified via `m68k-amigaos-nm`, no large stack arrays, no alignment quirks). **Depends on lib/libhubbub + lib/libparserutils + lib/libwapcaplet** (link order: `-ldom -lhubbub -lparserutils -lwapcaplet`). Memory-checker APPROVED with one CRITICAL-code / MID-practical finding (`_dom_event_targets_expand` at `src/core/node.c:2431` has unsafe realloc pattern that leaks the old buffer on growth failure -- low risk because HTML5 trees are shallow and the targets array rarely realloc-grows; deferred to upstream patch). Source-analyzer CLEAN. 47/47 tests pass on vamos `-C 68040 -s 4096 -m 8192`. **CRITICAL vamos resource sizing extension:** test binaries linking the FULL NetSurf-Vampire dep stack (libdom + libhubbub + libparserutils + libwapcaplet) need `__stack >= 1048576` AND `__MEMORY_STEP >= 1048576` -- 512 KB cookies (sufficient for libhubbub alone) NOT enough; libdom's larger code+data footprint pushes libnix's startup-time allocation past 512 KB. Symptom below threshold: pre-main Bus Error at PC=0x40a with NO stdout. **Mandatory consumer cleanup:** call `dom_namespace_finalise()` AND `lwc_iterate_strings(NULL, NULL)` AND `dom_node_unref(doc)` for every Document at exit -- AmigaOS `-noixemul` does NOT reclaim process memory. **Upstream libdom CharacterData mutation bug** (NULL-deref): calling `dom_characterdata_delete_data` / `dom_characterdata_replace_data` / `dom_text_split_text` on a parent-less character data node fires DOMSubtreeModified against `c->parent`==NULL and crashes via NULL vtable deref; the hubbub binding never hits this in practice (always appends text to a parent first) but direct API consumers must attach character data nodes to a parent BEFORE mutating. Documented in `lib/libdom/README.md`, `lib/libdom/MEMORY-AUDIT.md`, `lib/libdom/PERF-REPORT.md`, and amiga-kb pitfall. Required by future `lib/libsvgtiny/` and ultimately `ports/netsurf/`. Top-level: `make build-libdom`, `make test-libdom`.
- `lib/freetype/` — FreeType 2.13.3 TrueType/OpenType font rendering library (~486 KB). SDL_ttf-only 9-module subset (base, truetype, sfnt, autofit, smooth, pshinter, psaux, psnames, raster + helpers ftmm/ftglyph/ftbitmap/ftbbox). Built `-O0 -m68000 -noixemul -std=gnu99` default with per-file `-O1 -fno-strict-aliasing` on 5 audited hot-path files (`smooth/smooth.c` AA rasterizer, `raster/raster.c` mono scanline, `truetype/truetype.c` bytecode interpreter, `base/ftbitmap.c`, `base/ftbbox.c`). `ftbase.c` and `autofit.c` deferred at `-O0` — their TUs include large sub-files (ftstream.c, ftobjs.c, aflatin.c) that need dedicated struct-return audit before promotion. `ftoption.h` patched in-place to disable `USE_ZLIB` / `ENVIRONMENT_PROPERTIES` / `SVG` (command-line `-U` does not override header `#define`). Zero soft-float pulls — all arithmetic is 16.16 and 26.6 fixed-point integer math via libgcc `___muldi3`/`___divdi3`; no `__divsf3`/`__floatunsisf` references, so downstream consumers are not exposed to the `mathieeesingbas.library` crash family (crash-patterns #2 variant, PDR-012). 218 `_FT_*` exported symbols — full FreeType 2 API except the disabled options. No CHIP RAM allocation (`ftsystem.c` uses libnix heap → Fast). **Consumers must set `__stack >= 65536`** — the TrueType bytecode interpreter (`ttinterp.c`) + autofitter (`afloader.c`) push deep call stacks during glyph hinting; combined with AmigaOS's 2-4 KB hidden dos.library depth (crash-patterns #10), smaller stacks will Guru on complex hinted fonts. Downstream: paired with Aminet SDL_ttf 2.0.9 + libSDL.a (toolchain) to ship text rendering for SDL1 game ports (PDR-014 Lane B). Stage 5 tests at `tests/freetype/` pass 26/26 on vamos with `-s 256 -m 4096` (vamos default 8 KB stack and default memory both insufficient for font face loads).
- `site/` — Website source for amiport.platesteel.net
  - `site/css/style.css` — MUI warm gray design system (see DESIGN.md)
  - `site/index.html` — Landing page (hero terminal animation, featured packages, getting started, port request form)
  - `site/packages.html` — Package browser with search/filter/sort, rich detail view (porting notes, test gauge, limitations)
  - `site/stats.html` — Stats dashboard with SVG bar charts, category breakdown, publication timeline
  - `site/news.html` — News archive page — release announcements, project updates, behind-the-scenes notes. JS-driven from `site/data/news.json`, rendered by `site/js/news.js`.
  - `site/amiga.html` — HTML 3.2 page for classic Amiga browsers (IBrowse/AWeb). PHP-generated, table layout, <30KB, 640x480
  - `site/gaming.html` — Gaming portal landing. Dark hero + live FPS leaderboard, port-tile grid (featured + 9 others), live telemetry table, project stats, submit form, changelog, Tweaks panel (accent/layout/density/arcade). Loads `css/gaming.css` on top of `css/style.css` and `js/gaming.js`. Links to `games/<id>.html`.
  - `site/games/<id>.html` — Per-port detail page shell (10 total: julius, chocolate-doom, ccleste, 1oom, sdlpop, vanilla-conquer, fheroes2, reminiscence, another-world, opentyrian). Minimal shell with `<html data-game-id="...">`; all rendering via `js/gaming-detail.js`.
  - `site/css/gaming.css` — Gaming portal styles: hero-strip, leaderboard slab, port-tile grid, status pills, screenshot placeholder, filter bar, tweaks panel, arcade/dense modes, detail-page blocks (hero, tabs, keymap, compat, patch-viewer, issue list, kv-list, breadcrumb).
  - `site/js/gaming.js` — Gaming landing page logic. Vanilla JS (no framework). Port data, live-FPS ticker (1.4s jitter ±2), filter/sort, tile rendering, submit-form validation, tweaks panel.
  - `site/js/gaming-detail.js` — Detail-page renderer. Reads `data-game-id`, merges per-port overrides (Julius has hand-crafted details) with a generic `makeDetails()` fallback. Renders hero, screenshot strip, tabs (Overview/Controls/Issues/Compat/Changelog/Patches), and sidebar (Install/Requirements/See also).
  - `site/feed.php` — RSS 2.0 feed combining published packages **and news entries** (`site/data/news.json`), sorted by date. `?category=<cat>` filter narrows to packages only (news is project-wide).
  - `site/data/news.json` — Source of truth for site news. Flat JSON array of `{id, date, title, body, tags, url}`. ASCII-only. Appended via `/post-news` skill — do not hand-edit when the skill applies. `site/api/v1/activity.php` and `site/feed.php` read this file server-side; the browser reaches it via `site/api/v1/news.php` (see below) because `.htaccess` blocks direct `/data/` access.
  - `site/api/v1/news.php` — Public JSON proxy for `site/data/news.json`. Matches the same pattern as `packages.php` proxying `data/packages/*.json`. Validates JSON parses before echoing.
  - `site/js/packages.js` — Package browser logic + keyboard shortcuts (P/S/Esc//)
  - `site/js/stats.js` — Stats rendering with SVG chart generation (no charting library)
  - `site/js/news.js` — News archive renderer. Supports a markdown-lite subset in `body`: paragraphs (blank line), links (`[text](url)`), bold (`**x**`), inline code (`` `x` ``).
  - `site/js/terminal-anim.js` — Hero typing animation (respects prefers-reduced-motion)
  - `site/api/v1/` — PHP API endpoints (packages, stats, download, vote, request, activity — the activity endpoint merges news.json entries into the feed)
- `toolchain/` — Cross-compiler Docker images, build scripts, target profiles
- `toolchain/docker/Dockerfile.bebbo-gcc13` — GCC 13.3.0 C++17 cross-compiler image (builds on Linux, includes `reent.h` patch for libnix)
- `toolchain/keyinject/` — KeyInject: keyboard event injector for functional interactive testing via AddIEvents() (ADR-023)
- `toolchain/screenread/` — ScreenRead: ConUnit cursor reader for visual test cursor verification (ADR-025)
- `toolchain/amigactl/` — Vendored tbdye/amigactl LHA (v0.8.2, GPL-3.0) + `install-amigactld.script` for real-Amiga remote control over TCP. Channel A of the real-hardware test loop. Daemon installed on user's A2000 + Vampire V2 + X-Surf 100 + Roadshow at 192.168.1.215; reaches `python3 -m amigactl --host 192.168.1.215 <cmd>` from `/tmp/amigactld-extract/amigactl/client/` (or after `pipx install`). Capabilities: put/get/exec/run/tail/sysinfo/arexx/trace. See memory `reference_amigactl.md` and `project_real_hardware_loop.md` for the three-channel architecture (A=amigactl LIVE, B=serial+Sashimi pending cable, C=bgdbserver staged).
- `toolchain/bgdbserver/` — Vendored Stefan Franke (bebbo) bgdbserver v1.3 binary (GPL-3.0, AmigaOS HUNK, 18.9 KB) + `bgdbserver.readme` + `COPYING`. Channel C of the real-hardware test loop — gdb-server for source-level remote debugging of bebbo-gcc binaries on real AmigaOS. Pushed to `C:bgdbserver` on user's A2000 (per-debug-session, not auto-started). Companion to host-side `m68k-amigaos-gdb` 13.0.50 already in `amigadev/crosstools:m68k-amigaos` Docker image. Usage: on Amiga `bgdbserver Programs:.../prog args` (default port 2345), on host `m68k-amigaos-gdb prog` then `target remote 192.168.1.215:2345`. Requires the debug target compiled with **`-gstabs`** (older format, NOT bebbo-gcc 13.3's default DWARF) — full Channel C end-to-end pending an openttd debug-build session.
- `scripts/inject-keys.sh` — Host-side key injection via macOS osascript for visual tests (ADR-025). Batches all keystrokes into a single osascript call (~1.7s overhead)
- `docs/` — Architecture docs, API mapping tables, porting guide, tier classification
- `docs/references/adcd/` — Complete ADCD 2.1 in agent-optimized markdown (Libraries, Devices, Hardware, Amiga Mail, Autodocs)
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) converted to markdown — 68030 CPU internals, custom chip architecture, memory map, hardware programming
- `tests/` — Unit tests (shim/, emu/, console/, net/, common/, zlib/)
- `ports/` — Output directory for real ports (each port gets original/, ported/, Makefile, PORT.md)
- `ports/templates/` — Canonical templates for per-port artifacts (Makefile, PORT.md, .readme, directory structure, and `run-sdlgame.template` — mandatory launcher script for every Category 5 SDL game port, assigns WORK: to the install dir before launch. See `.claude/rules/known-pitfalls.md` "libSDL2-amigaos3 Game Ports Hardcode WORK:").
- `data/catalog.json` — Porting Tech Tree: candidate inventory, readiness scoring, shim unlock index, hardware profiles
- `scripts/catalog-score.py` — Catalog scoring, validation, status dashboard, next-candidate selection, diff reports

## Using the Pipeline — CRITICAL (ENFORCED BY HOOKS)

**Agent dispatch is MANDATORY.** A PreToolUse hook (`enforce-agents.sh`) warns on direct edits to `ported/*.c` files as a reminder to use pipeline agents. The real enforcement is via CLAUDE.md rules and `/port-project` GATE checks.

The `/port-project` skill has GATE checks — it will not proceed to the next stage until the current stage's agent has returned successfully.

**Library ports (`lib/<name>/`) use the same pipeline discipline as port ports.** There is no separate `/port-library` skill — apply the stages manually per `.claude/rules/library-pipeline.md`. Mandatory: KB query → source-analyzer → build → test-designer (library mode) → test-runner → memory-checker → perf-optimizer → docs. Do NOT `make -C lib/<name>` before source-analyzer has returned.

**Post-port quality skills:**
- `/extend-shim <function-name>` — Add a missing POSIX function to the shim library
- `/review-amiga <path>` — Amiga-specific code review (stack safety, BPTR handling, conventions)

**Site skills:**
- `/post-news` — Publish a news entry to `site/data/news.json` and deploy. Use for release announcements, project updates, and behind-the-scenes notes. Validates JSON + ASCII, dispatches `site-manager` to deploy, clears the activity cache. Do not hand-edit `site/data/news.json` when this skill applies — use the skill so deploy + cache clear happen atomically.

**Available agents:**
| Agent | When to dispatch |
|-------|-----------------|
| `aminet-researcher` | Before any port — check if it already exists |
| `source-analyzer` | Stage 1 — portability analysis |
| `code-transformer` | Stage 3 — source transformation |
| `build-manager` | Stage 4 — cross-compilation and error fixing |
| `test-runner` | Stage 5 — vamos testing |
| `port-coordinator` | **DEPRECATED** — cannot dispatch subagents. Orchestrate from main session instead, dispatching specialized agents directly. |
| `dependency-auditor` | Before complex ports — audit external library dependencies |
| `debug-agent` | When a port crashes at runtime — autonomous Enforcer-based crash diagnosis and fix loop |
| `memory-checker` | **Mandatory** Stage 6b — memory leak detection, double-free, allocation safety |
| `perf-optimizer` | **Mandatory** Stage 6c — 68k static analysis and optimization recommendations |
| `profiler` | Optional Stage 6d — empirical ReadEClock-based runtime measurement. Validates perf-optimizer findings |
| `hardware-expert` | Hardware architecture validation — on-demand consultant + proactive doc auditor. Dispatch when agents need hardware facts (address space, CPU variants, chipset capabilities). |
| `test-designer` | Two modes: **port mode** — designs FS-UAE test suites (`test-fsemu-cases.txt`); **library mode** — designs C unit test plans for `lib/<name>/` using `tests/shim/test_framework.h`. Both enforce the test-coverage-standard. |
| `aminet-publisher` | Publishing — curated, never automatic |
| `site-manager` | Website operations — deployment, manifest generation, security scanning, testing |
| `visual-test-expert` | Visual test authoring and debugging — SCRAPE/SCREEN_READ/EXPECT_TRAP_CURSOR (ADR-024/025) |
| `amiport-publisher` | Publish ports to amiport.platesteel.net — test-gated, never automatic |
| `catalog-engineer` | Catalog management — candidate enumeration, dry-run analysis, scoring, batch dispatch |
| `port-worker` | **Draft mode only** — self-contained porting worker for quick-pass batch dispatch in worktrees. Use specialized agents via `/batch-port-parallel` for production quality. |
| `regression-checker` | After shim/library changes — rebuild and test all affected ports to detect regressions |
| `sdl-game-helper` | SDL1/SDL2 game-port specialist. Dispatch for any new SDL game port (after `aminet-researcher`) or to perf-audit an existing one. Knows libSDL2-amigaos3 fast-path traps (`OS3_OpenWindowed` +64 padding, BitMapScale fallback, asm bswap32, libSDL2 `-O0` default), bebbo-gcc 13.3 libstdc++ ABI traps (`std::ostream<<int` Guru, `std::string operator+` at -O0, `std::this_thread::sleep_for` 20 ms granularity), frame-budget math, sprite-cache sizing, OS-cursor strategy, and the FS-UAE CPU/FPU compatibility matrix. Flexible-style specialist — adapts playbook to context. |

## Documentation Rules — IMPORTANT

**A change is not complete until all affected documentation is updated.** Full checklist with all 13 touchpoints is in `.claude/rules/documentation.md`. Key points:

- Skills/agents/libraries/ADRs: update CLAUDE.md, README.md, architecture.md, porting-guide.md, port-project skill
- Completed ports: update PORTS.md, README.md ports table
- Port updates (version/deps/tests): update catalog.json, site packages.json
- Cross-cutting changes: audit ALL touchpoints upfront (see rule for full matrix)

## First-Time Setup — MANDATORY

**Run this immediately after cloning.** It configures git hooks that enforce documentation consistency and port directory hygiene. Without this, commits will not be validated.

```bash
make setup             # Configure git hooks (REQUIRED — run first)
make setup-toolchain   # Install cross-compiler Docker image
```

## Build Instructions

```bash
make help              # Show all available targets
make setup             # Configure git hooks (run after cloning)
make setup-toolchain   # Install/pull cross-compiler (Docker)
make build-shim        # Build the POSIX shim library (Tier 1)
make build TARGET=examples/wc   # Build a specific port
make test TARGET=examples/wc    # Test via vamos
make test-shim         # Run POSIX shim library tests via vamos
make test-ports        # Test all production ports via vamos
make check-docs        # Validate agent references across all docs
make check-port-metadata  # Validate port metadata consistency (version, files, PORTS.md)
make check-arexx       # Validate ARexx files (non-ASCII, compound vars, syntax)
make build-keyinject   # Build KeyInject (keystroke injector for interactive tests)
make build-screenread  # Build ScreenRead (screen state reader for visual tests)
make clean             # Remove build artifacts
```

**Prerequisites:** Docker (for cross-compiler), Python + amitools (`pip install amitools`) for vamos testing.

## Versioning

Each port has two version components defined in its Makefile:

- **VERSION** — upstream version (e.g., `1.68`). Only changes when pulling new upstream source.
- **REVISION** — port revision (default `1`). Increment when `ported/`, Makefile, shim deps, or tests change but upstream version stays the same.

`common.mk` computes **DISPLAY_VERSION**: `VERSION` for revision 1, `VERSION-REVISION` for revision 2+ (e.g., `1.68-2`). DISPLAY_VERSION flows to:

- `$VER` string in source code
- `.readme` `Version:` field
- LHA filename (e.g., `grep-1.68-2.lha`)
- Website package display
- PORTS.md catalog

Revision 1 is implicit — never shown. Run `make check-port-metadata` to validate version consistency across all touchpoints.

## Toolchain

Primary: **bebbo/amiga-gcc** (`m68k-amigaos-gcc`) in Docker
Secondary: **VBCC** (`vc`) in Docker

**Two Docker images available:**
- `ghcr.io/bdgscotland/amiport-toolchain:latest` — GCC 6.5.0b (C89/C99/C++14). Default for all C ports.
- `ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest` — GCC 13.3.0 (full C++17). For OpenTTD and modern C++ ports. Built from `toolchain/docker/Dockerfile.bebbo-gcc13`. Includes `reent.h` patch for libnix/libstdc++ compatibility.

The toolchain scripts in `toolchain/scripts/` handle detection and invocation. Always use these scripts rather than calling compilers directly.

## Testing

Use **vamos** (from amitools) for CLI program testing (Categories 1-2) — it provides a virtual AmigaOS runtime without needing a full emulator. The `test-amiga` skill handles this.

For console UI apps (Category 3), network apps (Category 4), GUI programs, or hardware-dependent code, use **FS-UAE** with a configured AmigaOS 3.x installation. See ADR-014 for automated FS-UAE testing design.

For interactive console programs (Category 3+), the test harness supports `ITEST:` blocks that use **KeyInject** (`toolchain/keyinject/`) to inject keystrokes via `commodities.library/AddIEvents()` for functional tests (exit code verification). Interactive tests are skipped on vamos (KeyInject requires AmigaOS). See ADR-023.

For **visual verification** (ADR-024), use a **separate test file** (`test-fsemu-visual-cases.txt`) with `SCRAPE`, `EXPECT_AT row,col,text`, and `EXPECT_CURSOR row,col` directives. **Functional and visual tests MUST be separate FS-UAE passes** -- never mix them in one suite. Resource exhaustion at ~13 ITESTs is a hard wall. Run visual tests with `make test-fsemu TARGET=ports/<name> VISUAL=1` (passes `--visual` to `scripts/test-fsemu.sh`). The forked FS-UAE (`~/Developer/fs-uae/`) captures per-unit ANSI output; host-side `scripts/verify-screen.py` uses pyte for screen reconstruction. ARexx syntax validated by `scripts/check-arexx-syntax.py` / `make check-arexx`. Note: `CMD_WRITE` captures static display (file load, help text) but NOT interactive echo (typed chars, cursor movement).

For **visual test key injection** (ADR-025), visual ITEST blocks use **host-side injection** via `scripts/inject-keys.sh` instead of Amiga-side KeyInject. This sends keystrokes through macOS `osascript` (System Events) into FS-UAE's SDL input path -- the same path as physical keypresses. AddIEvents() does not reliably deliver to RAW mode programs in visual tests. The ARexx harness coordinates via sentinel files (`keys-request-N` / `keys-done-N`). The `CLEANUP:` directive sends quit keys after SCRAPE capture.

For **cursor position verification** (ADR-025), use `EXPECT_TRAP_CURSOR row,col` in visual test files for COOKED mode programs. This reads cursor position directly from the ConUnit struct via a custom FS-UAE trap (mode 150). Requires `SCREEN_READ` directive and the ScreenRead binary (`toolchain/screenread/`). For RAW mode programs (mg, less, nano), ConUnit cursor stays at (0,0) -- verify cursor via the program's status line using `EXPECT_AT` instead.

## Design System

Always read `DESIGN.md` before making any visual or UI decisions for the website (`site/`).
All font choices, colors, spacing, and aesthetic direction are defined there.
Do not deviate without explicit user approval.
In QA mode, flag any code that doesn't match DESIGN.md.

## Key References

**Shared Knowledge Base (amiga-kb via MCP):**

General Amiga reference docs are in the shared amiga-kb knowledge base (~44K vectors, ~2.6K graph nodes, ~3.5K edges). Use MCP tools instead of reading local files. All queries are tagged with `source_project: "amiport"` for cross-project analytics.

Query tools (read-only):
- `amiga_search` — hybrid vector+keyword search across all docs (RRF fusion)
- `amiga_api_lookup` — function/struct lookup with graph traversal and pitfall warnings
- `amiga_pitfalls_for` — known pitfalls for an API or concept
- `amiga_crash_diagnosis` — crash diagnosis from Guru codes
- `amiga_techniques_for` — demo/game techniques by topic, filterable by chipset (OCS/ECS/AGA)
- `amiga_recipe_lookup` — buildable code recipe with dependency chain and technique links
- `amiga_register_lookup` — custom chip register details + which techniques use it
- `amiga_check_compatibility` — check register overlap and chipset compatibility between techniques
- `amiga_port_briefing` — structured porting briefing from a project description (surfaces pitfalls, required libs, relevant docs)

Hydration tools (write — route universal knowledge here):
- `amiga_ingest_doc` — add/update a document (disk + vectors + graph)
- `amiga_add_pitfall` — add pitfall with auto-edge extraction
- `amiga_add_crash_pattern` — add crash pattern with graph links

Project intelligence:
- `amiga_add_plan` / `amiga_add_todo` / `amiga_update_status` — plan and track work across projects
- `amiga_add_dependency` / `amiga_get_work` / `amiga_get_blockers` — dependency chains and outstanding work

Intelligence (self-improvement):
- `amiga_coverage` — coverage report, query analytics, gap detection
- `amiga_suggest_links` — find orphan nodes, suggest missing graph edges
- `amiga_report_gap` — report missing knowledge for future enrichment
- `amiga_health` — health check all backing services

The amiga-kb MCP server must be running (`docker compose up -d` in the amiga-kb repo).

**Critical (project-specific, consult during every port):**
- `docs/posix-tiers.md` — Master POSIX tier classification (Tier 1/2/3 for every function)
- `docs/references/adcd/` — Complete ADCD 2.1 in markdown (also indexed in amiga-kb vectors)
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) — 68030 internals, custom chip architecture
- `docs/references/m68000-prm/` — Motorola M68000 Family Programmer's Reference Manual (646 pages)
- `docs/test-coverage-standard.md` — **Mandatory** test coverage requirements
- `.claude/skills/transform-source/references/transformation-rules.md` — Tier 1 transformation rules

**Skills for on-demand context loading:**
- `/amiga-api-lookup` — Loads ADCD reference. For simple lookups, prefer `amiga_api_lookup` MCP tool (faster, includes graph data and pitfall warnings).
- `/c89-reference` — C89/ANSI C constraints. No MCP equivalent (project-specific).
- `/write-arexx` — ARexx syntax reference. Also available via `amiga_search "arexx ..."`.
- `/crash-patterns` — Crash KB loader. For quick diagnosis, prefer `amiga_crash_diagnosis` MCP tool.
- `/libnix-reference` — libnix function list. Also available via `amiga_search "libnix ..."`.

**Skill injection:** Knowledge base skills are injected into agent definitions via the `skills:` frontmatter field. When an agent is dispatched, its injected skills are loaded into context automatically. See individual agent definitions in `.claude/agents/` for the injection matrix.
- `/extend-shim` — Invoke when adding new POSIX functions to the shim library.
- `/review-amiga` — Invoke for Amiga-specific code review.
- `/capture-learning` — Invoke when a bug, mistake, or process failure occurs. **Dual-writes** by default: project-local enforcement (hook/rule/agent/skill/memory — strongest) AND amiga-kb via `amiga_add_pitfall` / `amiga_add_crash_pattern` / `amiga_report_gap` when the learning is universal AmigaOS/68k/libnix/bebbo knowledge. Project-local knowledge (pipeline mechanics, site architecture, amiport conventions) stays local only. The skill file has the full YES/NO classification.

**Architecture & guides:** `docs/architecture.md`, `docs/porting-guide.md`, `docs/api-mapping.md`

**ADRs:** `docs/adr/008` (tiers), `009` (console), `010` (bsdsocket), `011` (categories), `014` (FS-UAE testing), `015` (CI/quality), `016` (debug agent), `017` (hooks enforcement), `018` (ADCD knowledge base), `019` (agent persona matrix), `020` (git hooks validation), `021` (design system — MUI warm gray), `022` (C99 compiler support), `023` (automated interactive testing), `024` (visual verification), `025` (screen read trap — interactive cursor verification), `026` (CPython port)

**PDRs:** `docs/pdr/001` (target audience), `002` (MVP wc port), `003` (ports directory + Aminet), `004` (Aminet research first), `005` (committed binaries), `006` (FS-UAE mandatory for all categories), `007` (design system redesign), `008` (SDL2 AmigaOS3 vision — delivered, partially superseded by PDR-014), `009` (hardware expansion + SDL), `013` (Dropbear SSH client), `014` (fold SDL game ports into amiport as Category 5)

**Shim references (in amiga-kb):** Use `amiga_search "bsd socket mapping"`, `amiga_search "newlib availability"` etc. ADCD FUNCTIONS.md and TYPES.md still local at `docs/references/adcd/`.

## Safety Hooks

The project enforces structural safety via hooks in `.claude/settings.json`:

- **`block-original-edits.sh`** — Blocks Edit/Write to `/original/`. Upstream source is read-only.
- **`block-root-files.sh`** — Blocks Edit/Write of non-config files in the project root.
- **`block-direct-gcc.sh`** — Blocks direct `m68k-amigaos-gcc`/`ld`/`as` calls. Forces use of `make` or toolchain scripts.
- **`warn-direct-port-build.sh`** — Warns on `make -C ports/` or `make TARGET=ports/` without using the build-manager agent. Allows `make test`, `make clean`, and `make -C lib/` (library builds).
- **`enforce-agents.sh`** — Warns on Edit/Write to `ported/*.c` files. Reminds to use code-transformer or debug-agent (warn-only — subagents use the same tools, so blocking would break the pipeline).
- **`verify-before-stop.sh`** — Reminds Claude to verify work before stopping.
- **`save-port-context.sh`** — On auto-compaction, injects active port names into context.
- **`check-toolchain.sh`** — Warns if Docker, vamos, lha, or jq are missing at session start.
- **`check-c89-comments.sh`** — Warns on C++ style `//` comments in C source files under `ports/` and `lib/`. bebbo-gcc with `-ansi` rejects them.
- **`auto-sync-catalog.sh`** — Auto-copies `data/catalog.json` to `site/data/catalog.json` after edits, preventing catalog drift.

## Git Hooks

The repo uses `.githooks/` for git hooks, configured by `make setup` (which runs `git config core.hooksPath .githooks`):

- **commit-msg**: Enforces conventional commit prefixes (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `chore:`, `ci:`, `perf:`, `style:`, `build:`). Allows merge commits.
- **pre-commit**: Runs `make check-docs`, `make check-port-metadata`, and `make check-arexx` (when .rexx files are staged) to validate agent references, port metadata consistency, and ARexx syntax. Also checks for stray root files, port directory hygiene, and non-ASCII in C source. Blocks commits that would introduce doc drift, metadata drift, or violate hygiene rules.
- **pre-push**: Builds the shim library and compiles all shim tests. Catches build/link breakage before it reaches origin. Runs expensive Docker cross-compilation (~10-15s).

**`make setup` is mandatory after cloning.** Without it, hook validation is skipped.

## Continuous Integration

CI runs on every push to main: builds all libs, runs all tests via vamos, validates docs and agent frontmatter, builds and tests all ports. See `.github/workflows/ci.yml`. Toolchain Docker image is cached on GHCR (`ghcr.io/bdgscotland/amiport-toolchain:latest`).
