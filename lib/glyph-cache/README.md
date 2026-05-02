# lib/glyph-cache

Generic LRU glyph cache for AmigaOS text-rendering ports. Caches 8-bit
alpha bitmaps keyed by `(face_id, codepoint, px_size, hint_flags)`.

Reusable by any text-rendering consumer: NetSurf (Phase 1 of PDR-XXX),
mg, less, future PDF viewer, future terminal emulator.

## Public API

See `include/amiport/glyph_cache.h`. Surface:

- `amiport_glyph_cache_create(size_t arena_bytes)` -> opaque `amiport_glyph_cache_t *`
- `amiport_glyph_cache_destroy(cache)`
- `amiport_glyph_cache_lookup(cache, face_id, codepoint, px_size, hint_flags, out)` -> 1=hit / 0=miss
- `amiport_glyph_cache_insert(cache, face_id, codepoint, px_size, hint_flags, glyph)` -> 1=ok / 0=too big
- `amiport_glyph_cache_get_stats(cache, out)` for tuning telemetry

Pure C99, no dynamic dependencies, links into libnix programs without
pulling AmigaOS libraries.

## v1 implementation notes

**Eviction policy.** When the bump-allocated arena fills, the cache is
wholesale reset rather than compacted. Subsequent lookups will miss
and repopulate from the renderer (FreeType, bullet, etc.).

This trades cache thrashing under pressure for implementation
simplicity. The LRU ordering is maintained even though it doesn't
drive partial eviction in v1 -- it's there to guide a future
compacting evictor if perf data shows it matters.

**Recommended arena size.** 256 KB to 2 MB depending on text load.
NetSurf body text on Wikipedia uses approximately 80 unique glyphs per
page; at 16 px x 16 px x 1 byte alpha = 256 bytes per glyph =
approximately 20 KB working set. 256 KB gives roughly 10x headroom
across a typical browsing session.

**Threading.** None. AmigaOS is single-threaded; the cache structure
holds a static state cookie that races would corrupt. Do not share a
cache between exec.library tasks.

**Alignment.** The bump-pointer arena rounds each insert up to a
4-byte boundary so that subsequent struct writes land on a properly
aligned word. This is mandatory on 68k where `offsetof()` returns 2,
not 4 or 8 (see project crash-patterns #15, KB pitfall "68k Alignment
Is 2, Not 4 or 8").

## Build

```bash
make -C lib/glyph-cache
```

Produces `libglyphcache.a` (built `-O0 -m68000`, vamos-compatible).
Promote to `-O1` only after a full FS-UAE audit cycle (see project
crash-patterns #16 / "Default to -O0 for Bundled Libraries Until
Proven Safe").

## Test

```bash
make -C tests/glyph-cache run
```

Runs the 6-test suite under vamos. Coverage:
- create/destroy lifecycle
- insert + lookup-hit roundtrip
- lookup-miss returns 0 (empty cache, after eviction)
- multiple-distinct-glyph insert
- LRU eviction when arena fills
- stats counters (hit/miss/eviction) increment correctly

## Consumers

- `ports/netsurf/` (Phase 1) -- FreeType + AMMX glyph compositor
- Future text-rendering ports -- mg, less, PDF viewer, terminal emulator

When integrating, the cache is single-instance per process. Allocate
one at init, free at exit. The arena size is the primary tunable;
collect `_get_stats()` data on real workloads to refine.
