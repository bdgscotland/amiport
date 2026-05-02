/* lib/glyph-cache/src/glyph_cache.c -- LRU glyph cache implementation.
 *
 * Task 10 stub: create/destroy only. Real implementation in Task 11-12.
 */

#include <amiport/glyph_cache.h>
#include <stdlib.h>
#include <string.h>

/* Opaque cache struct (stub version -- real fields added in Task 11). */
struct amiport_glyph_cache {
    size_t arena_bytes;
    /* Full hash table + LRU list in Task 11-12 */
};

amiport_glyph_cache_t *amiport_glyph_cache_create(size_t arena_bytes) {
    amiport_glyph_cache_t *cache;

    if (arena_bytes == 0) {
        return NULL;
    }

    cache = (amiport_glyph_cache_t *)malloc(sizeof(amiport_glyph_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->arena_bytes = arena_bytes;
    /* Arena allocation in Task 11 */

    return cache;
}

void amiport_glyph_cache_destroy(amiport_glyph_cache_t *cache) {
    if (!cache) {
        return;
    }
    /* Arena deallocation in Task 11 */
    free(cache);
}

/* Stub lookup/insert/stats -- implemented in Task 11-12 */
int amiport_glyph_cache_lookup(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                amiport_glyph_t *out_glyph) {
    (void)cache; (void)face_id; (void)codepoint; (void)px_size;
    (void)hint_flags; (void)out_glyph;
    return 0; /* Always miss in stub */
}

int amiport_glyph_cache_insert(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                const amiport_glyph_t *glyph) {
    (void)cache; (void)face_id; (void)codepoint; (void)px_size;
    (void)hint_flags; (void)glyph;
    return 0; /* Always fail in stub */
}

void amiport_glyph_cache_get_stats(const amiport_glyph_cache_t *cache,
                                    amiport_glyph_cache_stats_t *out) {
    if (!cache || !out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->arena_bytes = cache->arena_bytes;
}
