/* amiport/glyph_cache.h -- generic LRU glyph cache for AmigaOS ports.
 *
 * Reusable by any text-rendering port (NetSurf, mg, less, future PDF
 * viewer, terminal emulator). Caches 8-bit alpha bitmaps keyed by
 * (face_id, codepoint, px_size, hint_flags).
 *
 * Storage: pre-sized slab arena allocated at create() time. Glyph
 * bitmaps allocated bump-pointer style; LRU eviction reclaims oldest
 * entries when the arena fills.
 *
 * Thread-safety: NONE. AmigaOS is single-threaded; this is fine.
 *
 * No dynamic dependencies -- pure C, links into libnix programs without
 * pulling additional libraries.
 */

#ifndef AMIPORT_GLYPH_CACHE_H
#define AMIPORT_GLYPH_CACHE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct amiport_glyph_cache amiport_glyph_cache_t;

typedef struct {
    const uint8_t *bitmap;   /* 8-bit alpha, owned by cache */
    int32_t        advance_x_q16; /* 16.16 fixed-point */
    int16_t        bearing_x;
    int16_t        bearing_y;
    uint16_t       width;
    uint16_t       height;
    uint16_t       stride;   /* bytes per row */
} amiport_glyph_t;

/* Create a cache with the given arena size in bytes.
 * Returns NULL on allocation failure.
 * Recommended sizes: 256 KB - 2 MB depending on expected text load.
 */
amiport_glyph_cache_t *amiport_glyph_cache_create(size_t arena_bytes);

/* Free the cache and its arena. Safe to pass NULL. */
void amiport_glyph_cache_destroy(amiport_glyph_cache_t *cache);

/* Look up a glyph by composite key.
 * Returns 1 on hit (out_glyph filled in), 0 on miss.
 * Hits update the LRU ordering.
 */
int amiport_glyph_cache_lookup(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                amiport_glyph_t *out_glyph);

/* Insert a glyph. Copies the bitmap into the cache's arena.
 * Returns 1 on success, 0 if the glyph is larger than the arena
 * can ever hold (caller should bypass cache for this glyph).
 * Triggers LRU eviction as needed to make room.
 */
int amiport_glyph_cache_insert(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                const amiport_glyph_t *glyph);

/* Statistics (for tuning). */
typedef struct {
    size_t arena_bytes;
    size_t arena_used;
    size_t entry_count;
    size_t hit_count;
    size_t miss_count;
    size_t eviction_count;
} amiport_glyph_cache_stats_t;

void amiport_glyph_cache_get_stats(const amiport_glyph_cache_t *cache,
                                    amiport_glyph_cache_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_GLYPH_CACHE_H */
