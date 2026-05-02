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

/* ----- Main ----- */

int main(void) {
    printf("=== lib/glyph-cache unit tests ===\n");
    RUN_TEST(create_destroy);
    return test_summary();
}
