/* lib/glyph-cache/src/glyph_cache.c -- LRU glyph cache implementation.
 *
 * Hash table with linear probing + LRU doubly-linked list.
 * Arena-based bitmap storage (bump pointer, reclaimed via LRU eviction).
 */

#include <amiport/glyph_cache.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 512  /* Power of 2 for fast mod via & */
#define HASH_MASK (HASH_TABLE_SIZE - 1)

typedef struct cache_entry cache_entry_t;

struct cache_entry {
    uint32_t        face_id;
    uint32_t        codepoint;
    uint16_t        px_size;
    uint16_t        hint_flags;
    amiport_glyph_t glyph;        /* bitmap points into arena */
    cache_entry_t  *lru_prev;     /* LRU doubly-linked list */
    cache_entry_t  *lru_next;
    unsigned char   occupied;
};

struct amiport_glyph_cache {
    cache_entry_t  hash_table[HASH_TABLE_SIZE];
    unsigned char *arena;
    size_t         arena_bytes;
    size_t         arena_used;
    cache_entry_t *lru_head;      /* Most recently used */
    cache_entry_t *lru_tail;      /* Least recently used */
    size_t         entry_count;
    size_t         hit_count;
    size_t         miss_count;
    size_t         eviction_count;
};

/* ----- Arena Allocation with 4-byte alignment (pitfall #15) ----- */

static unsigned char *arena_alloc(amiport_glyph_cache_t *cache, size_t bytes) {
    size_t aligned_used;
    unsigned char *ptr;

    /* Round up to 4-byte boundary (68k struct alignment requirement) */
    aligned_used = (cache->arena_used + 3U) & ~3U;

    if (aligned_used + bytes > cache->arena_bytes) {
        return NULL;
    }

    ptr = cache->arena + aligned_used;
    cache->arena_used = aligned_used + bytes;
    return ptr;
}

/* ----- Hash Function ----- */

static unsigned int hash_key(uint32_t face_id, uint32_t codepoint,
                              uint16_t px_size, uint16_t hint_flags) {
    unsigned int h = (unsigned int)face_id;
    h ^= (unsigned int)codepoint * 31U;
    h ^= (unsigned int)px_size * 7U;
    h ^= (unsigned int)hint_flags * 13U;
    return h & HASH_MASK;
}

static int keys_equal(const cache_entry_t *e, uint32_t face_id, uint32_t codepoint,
                      uint16_t px_size, uint16_t hint_flags) {
    return (e->face_id == face_id && e->codepoint == codepoint &&
            e->px_size == px_size && e->hint_flags == hint_flags);
}

/* ----- LRU List Operations ----- */

static void lru_remove(amiport_glyph_cache_t *cache, cache_entry_t *e) {
    if (e->lru_prev) {
        e->lru_prev->lru_next = e->lru_next;
    } else {
        cache->lru_head = e->lru_next;
    }

    if (e->lru_next) {
        e->lru_next->lru_prev = e->lru_prev;
    } else {
        cache->lru_tail = e->lru_prev;
    }

    e->lru_prev = NULL;
    e->lru_next = NULL;
}

static void lru_push_front(amiport_glyph_cache_t *cache, cache_entry_t *e) {
    e->lru_prev = NULL;
    e->lru_next = cache->lru_head;

    if (cache->lru_head) {
        cache->lru_head->lru_prev = e;
    } else {
        cache->lru_tail = e;
    }

    cache->lru_head = e;
}

static void lru_touch(amiport_glyph_cache_t *cache, cache_entry_t *e) {
    if (e == cache->lru_head) {
        return; /* Already MRU */
    }
    lru_remove(cache, e);
    lru_push_front(cache, e);
}

/* ----- Create / Destroy ----- */

amiport_glyph_cache_t *amiport_glyph_cache_create(size_t arena_bytes) {
    amiport_glyph_cache_t *cache;
    size_t i;

    if (arena_bytes == 0) {
        return NULL;
    }

    cache = (amiport_glyph_cache_t *)malloc(sizeof(amiport_glyph_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->arena = (unsigned char *)malloc(arena_bytes);
    if (!cache->arena) {
        free(cache);
        return NULL;
    }

    cache->arena_bytes = arena_bytes;
    cache->arena_used = 0;
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    cache->entry_count = 0;
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->eviction_count = 0;

    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        cache->hash_table[i].occupied = 0;
    }

    return cache;
}

void amiport_glyph_cache_destroy(amiport_glyph_cache_t *cache) {
    if (!cache) {
        return;
    }
    free(cache->arena);
    free(cache);
}

/* ----- Lookup ----- */

int amiport_glyph_cache_lookup(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                amiport_glyph_t *out_glyph) {
    unsigned int idx;
    unsigned int probe;

    if (!cache || !out_glyph) {
        return 0;
    }

    idx = hash_key(face_id, codepoint, px_size, hint_flags);

    for (probe = 0; probe < HASH_TABLE_SIZE; probe++) {
        cache_entry_t *e = &cache->hash_table[(idx + probe) & HASH_MASK];

        if (!e->occupied) {
            cache->miss_count++;
            return 0; /* Not found */
        }

        if (keys_equal(e, face_id, codepoint, px_size, hint_flags)) {
            *out_glyph = e->glyph;
            lru_touch(cache, e);
            cache->hit_count++;
            return 1;
        }
    }

    cache->miss_count++;
    return 0; /* Table full or not found after full scan */
}

/* ----- Insert ----- */

int amiport_glyph_cache_insert(amiport_glyph_cache_t *cache,
                                uint32_t face_id,
                                uint32_t codepoint,
                                uint16_t px_size,
                                uint16_t hint_flags,
                                const amiport_glyph_t *glyph) {
    unsigned int idx;
    unsigned int probe;
    cache_entry_t *slot;
    unsigned char *bitmap_copy;
    size_t bitmap_bytes;

    if (!cache || !glyph) {
        return 0;
    }

    bitmap_bytes = (size_t)glyph->stride * glyph->height;

    /* Reject glyphs larger than total arena (can never fit) */
    if (bitmap_bytes > cache->arena_bytes) {
        return 0;
    }

    /* Allocate bitmap in arena */
    bitmap_copy = arena_alloc(cache, bitmap_bytes);
    if (!bitmap_copy) {
        /* LRU eviction in Task 12 */
        return 0;
    }

    memcpy(bitmap_copy, glyph->bitmap, bitmap_bytes);

    /* Find empty slot via linear probing */
    idx = hash_key(face_id, codepoint, px_size, hint_flags);

    for (probe = 0; probe < HASH_TABLE_SIZE; probe++) {
        slot = &cache->hash_table[(idx + probe) & HASH_MASK];

        if (!slot->occupied) {
            /* Found empty slot */
            slot->face_id = face_id;
            slot->codepoint = codepoint;
            slot->px_size = px_size;
            slot->hint_flags = hint_flags;
            slot->glyph = *glyph;
            slot->glyph.bitmap = bitmap_copy;
            slot->occupied = 1;

            lru_push_front(cache, slot);
            cache->entry_count++;
            return 1;
        }

        if (keys_equal(slot, face_id, codepoint, px_size, hint_flags)) {
            /* Duplicate insert -- update in place */
            slot->glyph = *glyph;
            slot->glyph.bitmap = bitmap_copy;
            lru_touch(cache, slot);
            return 1;
        }
    }

    /* Hash table full -- need eviction (Task 12) */
    return 0;
}

/* ----- Stats ----- */

void amiport_glyph_cache_get_stats(const amiport_glyph_cache_t *cache,
                                    amiport_glyph_cache_stats_t *out) {
    if (!cache || !out) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->arena_bytes = cache->arena_bytes;
    out->arena_used = cache->arena_used;
    out->entry_count = cache->entry_count;
    out->hit_count = cache->hit_count;
    out->miss_count = cache->miss_count;
    out->eviction_count = cache->eviction_count;
}
