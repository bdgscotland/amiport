/* tests/glyph-cache/test_glyph_cache.c -- unit tests for lib/glyph-cache
 * Using test_framework.h patterns from tests/shim/.
 */

#include <stdio.h>
#include <amiport/glyph_cache.h>
#include "../shim/test_framework.h"

long __stack = 262144;           /* 256 KB stack for vamos + real AmigaOS */
unsigned long __MEMORY_STEP = 262144; /* libnix default 4 MB breaks vamos */

/* ----- Test: create + destroy ----- */

TEST(create_destroy) {
    amiport_glyph_cache_t *cache = amiport_glyph_cache_create(256 * 1024);
    ASSERT_NOT_NULL(cache);
    amiport_glyph_cache_destroy(cache);
}

/* ----- Test: insert + lookup hit ----- */

TEST(insert_lookup_hit) {
    amiport_glyph_cache_t *cache = amiport_glyph_cache_create(256 * 1024);
    amiport_glyph_t glyph_in, glyph_out;
    uint8_t bitmap_data[16] = {0x11, 0x22, 0x33, 0x44};
    int result;

    /* Insert a small glyph */
    glyph_in.bitmap = bitmap_data;
    glyph_in.width = 4;
    glyph_in.height = 4;
    glyph_in.stride = 4;
    glyph_in.advance_x_q16 = 0x40000; /* 4.0 in Q16 */
    glyph_in.bearing_x = 0;
    glyph_in.bearing_y = 0;

    result = amiport_glyph_cache_insert(cache, 1, 'A', 16, 0, &glyph_in);
    ASSERT_EQ(result, 1);

    /* Lookup should hit */
    result = amiport_glyph_cache_lookup(cache, 1, 'A', 16, 0, &glyph_out);
    ASSERT_EQ(result, 1);
    ASSERT_NOT_NULL(glyph_out.bitmap);
    ASSERT_EQ(glyph_out.width, 4);
    ASSERT_EQ(glyph_out.height, 4);

    amiport_glyph_cache_destroy(cache);
}

/* ----- Test: lookup miss ----- */

TEST(lookup_miss) {
    amiport_glyph_cache_t *cache = amiport_glyph_cache_create(256 * 1024);
    amiport_glyph_t glyph_out;
    int result;

    /* Lookup non-existent glyph */
    result = amiport_glyph_cache_lookup(cache, 99, 'Z', 99, 0, &glyph_out);
    ASSERT_EQ(result, 0);

    amiport_glyph_cache_destroy(cache);
}

/* ----- Test: insert multiple distinct glyphs ----- */

TEST(insert_multiple) {
    amiport_glyph_cache_t *cache = amiport_glyph_cache_create(256 * 1024);
    amiport_glyph_t glyph_in, glyph_out;
    uint8_t bitmap_a[16], bitmap_b[16];
    int result;

    /* Insert glyph A */
    glyph_in.bitmap = bitmap_a;
    glyph_in.width = 4;
    glyph_in.height = 4;
    glyph_in.stride = 4;
    glyph_in.advance_x_q16 = 0x40000;
    glyph_in.bearing_x = 0;
    glyph_in.bearing_y = 0;

    result = amiport_glyph_cache_insert(cache, 1, 'A', 16, 0, &glyph_in);
    ASSERT_EQ(result, 1);

    /* Insert glyph B (different codepoint) */
    glyph_in.bitmap = bitmap_b;
    result = amiport_glyph_cache_insert(cache, 1, 'B', 16, 0, &glyph_in);
    ASSERT_EQ(result, 1);

    /* Both should be retrievable */
    result = amiport_glyph_cache_lookup(cache, 1, 'A', 16, 0, &glyph_out);
    ASSERT_EQ(result, 1);

    result = amiport_glyph_cache_lookup(cache, 1, 'B', 16, 0, &glyph_out);
    ASSERT_EQ(result, 1);

    amiport_glyph_cache_destroy(cache);
}

/* ----- Test: arena full triggers eviction (flush-all strategy) ----- */

TEST(eviction_arena_full) {
    amiport_glyph_cache_t *cache;
    amiport_glyph_t glyph_in, glyph_out;
    amiport_glyph_cache_stats_t stats;
    uint8_t bitmap[1024];
    int result;
    int i;

    /* Small arena: 3 KB (fits exactly 3 glyphs of 1 KB each) */
    cache = amiport_glyph_cache_create(3 * 1024);
    ASSERT_NOT_NULL(cache);

    glyph_in.bitmap = bitmap;
    glyph_in.width = 32;
    glyph_in.height = 32;
    glyph_in.stride = 32;
    glyph_in.advance_x_q16 = 0x40000;
    glyph_in.bearing_x = 0;
    glyph_in.bearing_y = 0;

    /* Fill arena with glyphs 'A', 'B', 'C' */
    for (i = 0; i < 3; i++) {
        result = amiport_glyph_cache_insert(cache, 1, 'A' + i, 16, 0, &glyph_in);
        ASSERT_EQ(result, 1);
    }

    /* Verify all present */
    result = amiport_glyph_cache_lookup(cache, 1, 'A', 16, 0, &glyph_out);
    ASSERT_EQ(result, 1);

    /* Insert 'D' -- arena full, flushes all entries (A, B, C) */
    result = amiport_glyph_cache_insert(cache, 1, 'D', 16, 0, &glyph_in);
    ASSERT_EQ(result, 1);

    /* After flush: only 'D' should be present */
    result = amiport_glyph_cache_lookup(cache, 1, 'D', 16, 0, &glyph_out);
    ASSERT_EQ(result, 1);

    /* 'A', 'B', 'C' should be gone */
    result = amiport_glyph_cache_lookup(cache, 1, 'A', 16, 0, &glyph_out);
    ASSERT_EQ(result, 0);
    result = amiport_glyph_cache_lookup(cache, 1, 'B', 16, 0, &glyph_out);
    ASSERT_EQ(result, 0);
    result = amiport_glyph_cache_lookup(cache, 1, 'C', 16, 0, &glyph_out);
    ASSERT_EQ(result, 0);

    /* Verify eviction count */
    amiport_glyph_cache_get_stats(cache, &stats);
    ASSERT_EQ(stats.eviction_count, 3); /* A, B, C evicted */

    amiport_glyph_cache_destroy(cache);
}

/* ----- Test: stats tracking ----- */

TEST(stats_tracking) {
    amiport_glyph_cache_t *cache = amiport_glyph_cache_create(256 * 1024);
    amiport_glyph_cache_stats_t stats;
    amiport_glyph_t glyph_in, glyph_out;
    uint8_t bitmap[16];

    glyph_in.bitmap = bitmap;
    glyph_in.width = 4;
    glyph_in.height = 4;
    glyph_in.stride = 4;
    glyph_in.advance_x_q16 = 0;
    glyph_in.bearing_x = 0;
    glyph_in.bearing_y = 0;

    amiport_glyph_cache_insert(cache, 1, 'A', 16, 0, &glyph_in);
    amiport_glyph_cache_lookup(cache, 1, 'A', 16, 0, &glyph_out); /* hit */
    amiport_glyph_cache_lookup(cache, 1, 'Z', 16, 0, &glyph_out); /* miss */

    amiport_glyph_cache_get_stats(cache, &stats);

    ASSERT_EQ(stats.entry_count, 1);
    ASSERT_EQ(stats.hit_count, 1);
    ASSERT_EQ(stats.miss_count, 1);
    ASSERT_EQ(stats.arena_bytes, 256 * 1024);

    amiport_glyph_cache_destroy(cache);
}

/* ----- Main ----- */

int main(void) {
    printf("=== lib/glyph-cache unit tests ===\n");
    RUN_TEST(create_destroy);
    RUN_TEST(insert_lookup_hit);
    RUN_TEST(lookup_miss);
    RUN_TEST(insert_multiple);
    RUN_TEST(eviction_arena_full);
    RUN_TEST(stats_tracking);
    return test_summary();
}
