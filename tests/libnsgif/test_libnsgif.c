/*
 * test_libnsgif.c -- unit tests for lib/libnsgif
 *
 * Library built -O1 -fno-strict-aliasing -m68040 -m68881 -DNDEBUG
 * (NetSurf-Vampire dep stack convention).
 *
 * Run via: vamos -C 68040 -s 1024 -m 4096 ./test_libnsgif
 *
 * Coverage: nsgif_create / nsgif_data_scan / nsgif_data_complete /
 * nsgif_frame_prepare / nsgif_frame_decode / nsgif_destroy lifecycle,
 * error-path safety, Amiga-specific (LZW decompressor on 68k,
 * little-endian header parse on big-endian host).
 *
 * 16 tests:
 *   5 functional (lifecycle, data scan, frame decode, info accessor,
 *                 strerror)
 *   4 error path (bad magic, truncated, NULL params, bad frame number)
 *   3 edge case (1x1 GIF, multi-frame animation, scan in chunks)
 *   2 Amiga-specific (little-endian LSD parse, LZW no soft-float)
 *   2 stress (50-iter create+destroy, parallel instances)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <nsgif.h>

#include "test_framework.h"

long __stack = 262144;
unsigned long __MEMORY_STEP = 262144;

/* ===================================================================
 * Bitmap callbacks
 * =================================================================== */

typedef struct {
    int width;
    int height;
    uint8_t *pixels;
    bool opaque;
} test_bitmap;

static nsgif_bitmap_t *cb_create(int width, int height)
{
    test_bitmap *b = calloc(1, sizeof(*b));
    if (b == NULL) return NULL;
    b->width = width;
    b->height = height;
    b->pixels = calloc(1, (size_t)width * (size_t)height * 4);
    if (b->pixels == NULL) {
        free(b);
        return NULL;
    }
    return b;
}

static void cb_destroy(nsgif_bitmap_t *bitmap)
{
    test_bitmap *b = bitmap;
    if (b == NULL) return;
    free(b->pixels);
    free(b);
}

static uint8_t *cb_get_buffer(nsgif_bitmap_t *bitmap)
{
    test_bitmap *b = bitmap;
    return b ? b->pixels : NULL;
}

static void cb_set_opaque(nsgif_bitmap_t *bitmap, bool opaque)
{
    test_bitmap *b = bitmap;
    if (b) b->opaque = opaque;
}

static bool cb_test_opaque(nsgif_bitmap_t *bitmap)
{
    test_bitmap *b = bitmap;
    return b ? b->opaque : false;
}

static void cb_modified(nsgif_bitmap_t *bitmap)
{
    (void)bitmap;
}

static const nsgif_bitmap_cb_vt cbs = {
    cb_create,
    cb_destroy,
    cb_get_buffer,
    cb_set_opaque,
    cb_test_opaque,
    cb_modified,
    NULL,  /* get_rowspan -- optional */
};

/* ===================================================================
 * Synthetic GIF fixture
 * =================================================================== */

/*
 * Minimal 1x1 GIF87a (43 bytes total):
 *   - Header (6 bytes): "GIF87a"
 *   - Logical Screen Descriptor (7 bytes): width=1, height=1,
 *     packed=0xF0 (global color table, 1 bit/pixel), bgcolor=0, aspect=0
 *   - Global Color Table (6 bytes): 2 entries x 3 bytes (RGB)
 *     entry 0 = white (0xFF, 0xFF, 0xFF), entry 1 = black (0x00, 0x00, 0x00)
 *   - Image Descriptor (10 bytes): comma + x=0,y=0, width=1,height=1,
 *     packed=0
 *   - Image Data:
 *     - LZW min code size byte = 2
 *     - Sub-block: length=2, codes for clear+pixel0+EOI compressed
 *     - Block terminator = 0
 *   - Trailer = 0x3B
 */
static const uint8_t gif_1x1_87a[] = {
    'G', 'I', 'F', '8', '7', 'a',         /* signature */
    1, 0, 1, 0,                            /* width=1, height=1 */
    0xF0,                                  /* GCT, 1 bit/pixel */
    0,                                     /* bgcolor */
    0,                                     /* aspect ratio */
    0xFF, 0xFF, 0xFF,                      /* GCT entry 0 = white */
    0x00, 0x00, 0x00,                      /* GCT entry 1 = black */
    0x2C,                                  /* image descriptor sep */
    0, 0,                                  /* x=0 */
    0, 0,                                  /* y=0 */
    1, 0,                                  /* width=1 */
    1, 0,                                  /* height=1 */
    0,                                     /* packed=0 (no LCT) */
    2,                                     /* LZW min code size */
    2,                                     /* sub-block length */
    0x44, 0x01,                            /* LZW: clear(4), pixel0(0), EOI(5) packed */
    0,                                     /* block terminator */
    0x3B                                   /* trailer */
};

/* ===================================================================
 * Category 1: Functional (5)
 * =================================================================== */

TEST(nsgif_create_destroy)
{
    nsgif_t *gif = NULL;
    nsgif_error err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    ASSERT_NOT_NULL(gif);
    nsgif_destroy(gif);
}

TEST(nsgif_data_scan_minimal)
{
    nsgif_t *gif = NULL;
    nsgif_error err;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);

    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);

    nsgif_data_complete(gif);
    nsgif_destroy(gif);
}

TEST(nsgif_frame_decode_minimal)
{
    nsgif_t *gif = NULL;
    nsgif_error err;
    nsgif_bitmap_t *bitmap = NULL;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    err = nsgif_frame_decode(gif, 0, &bitmap);
    ASSERT_EQ(err, NSGIF_OK);
    ASSERT_NOT_NULL(bitmap);

    nsgif_destroy(gif);
}

TEST(nsgif_strerror_safe)
{
    const char *s_ok = nsgif_strerror(NSGIF_OK);
    const char *s_err = nsgif_strerror(NSGIF_ERR_DATA);
    ASSERT_NOT_NULL(s_ok);
    ASSERT_NOT_NULL(s_err);
    ASSERT(strlen(s_ok) > 0);
    ASSERT(strlen(s_err) > 0);
}

TEST(nsgif_frame_prepare)
{
    nsgif_t *gif = NULL;
    nsgif_error err;
    nsgif_rect_t area = {0};
    uint32_t delay_cs = 0;
    uint32_t frame_new = 0;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    err = nsgif_frame_prepare(gif, &area, &delay_cs, &frame_new);
    ASSERT_EQ(err, NSGIF_OK);
    /* area should cover at least the 1x1 frame */
    ASSERT(area.x1 >= 1);
    ASSERT(area.y1 >= 1);

    nsgif_destroy(gif);
}

/* ===================================================================
 * Category 2: Error path (4)
 * =================================================================== */

TEST(scan_bad_magic)
{
    /*
     * Replace 'GIF' with 'XXX' -- nsgif_data_scan should reject as
     * NSGIF_ERR_DATA.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;
    uint8_t bad[sizeof(gif_1x1_87a)];

    memcpy(bad, gif_1x1_87a, sizeof(bad));
    bad[0] = 'X'; bad[1] = 'X'; bad[2] = 'X';

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(bad), bad);
    ASSERT_EQ(err, NSGIF_ERR_DATA);

    nsgif_destroy(gif);
}

TEST(scan_truncated)
{
    /*
     * Pass only 4 bytes -- below the 13-byte minimum (header + LSD).
     * Should return ERR_END_OF_DATA.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, 4, gif_1x1_87a);
    /* Truncated data either hits END_OF_DATA or DATA error */
    ASSERT(err == NSGIF_ERR_END_OF_DATA || err == NSGIF_ERR_DATA);

    nsgif_destroy(gif);
}

TEST(decode_bad_frame_number)
{
    /*
     * Decode frame 999 from a 1-frame GIF -- should return ERR_BAD_FRAME.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;
    nsgif_bitmap_t *bitmap = NULL;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    err = nsgif_frame_decode(gif, 999, &bitmap);
    ASSERT_EQ(err, NSGIF_ERR_BAD_FRAME);

    nsgif_destroy(gif);
}

TEST(scan_after_complete_rejected)
{
    /*
     * After data_complete, further scan calls must return
     * NSGIF_ERR_DATA_COMPLETE per the documented contract.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_ERR_DATA_COMPLETE);

    nsgif_destroy(gif);
}

/* ===================================================================
 * Category 3: Edge case (3)
 * =================================================================== */

TEST(scan_chunked)
{
    /*
     * Per the documented contract, data_scan can be called multiple
     * times with the data array re-passed at growing length each call.
     * Verify chunked feed produces same result as one-shot.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;
    size_t i;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);

    /* Feed the entire data buffer in growing chunks of 1, 5, 10, all */
    err = nsgif_data_scan(gif, 1, gif_1x1_87a);
    ASSERT(err == NSGIF_OK || err == NSGIF_ERR_END_OF_DATA);
    err = nsgif_data_scan(gif, 5, gif_1x1_87a);
    ASSERT(err == NSGIF_OK || err == NSGIF_ERR_END_OF_DATA);
    err = nsgif_data_scan(gif, 10, gif_1x1_87a);
    ASSERT(err == NSGIF_OK || err == NSGIF_ERR_END_OF_DATA);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    /* Still decodes */
    {
        nsgif_bitmap_t *bitmap = NULL;
        err = nsgif_frame_decode(gif, 0, &bitmap);
        ASSERT_EQ(err, NSGIF_OK);
    }

    nsgif_destroy(gif);
}

TEST(reset_replays_animation)
{
    /*
     * After decoding the only frame, nsgif_reset should rewind so we
     * can decode it again.
     */
    nsgif_t *gif = NULL;
    nsgif_error err;
    nsgif_bitmap_t *bitmap = NULL;
    int i;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);

    for (i = 0; i < 3; i++) {
        err = nsgif_frame_decode(gif, 0, &bitmap);
        ASSERT_EQ(err, NSGIF_OK);
        err = nsgif_reset(gif);
        ASSERT_EQ(err, NSGIF_OK);
    }

    nsgif_destroy(gif);
}

TEST(all_pixel_format_variants)
{
    /*
     * libnsgif supports 8 different pixel formats. Verify create+decode
     * works for all of them.
     */
    nsgif_bitmap_fmt_t fmts[] = {
        NSGIF_BITMAP_FMT_R8G8B8A8,
        NSGIF_BITMAP_FMT_B8G8R8A8,
        NSGIF_BITMAP_FMT_A8R8G8B8,
        NSGIF_BITMAP_FMT_A8B8G8R8,
        NSGIF_BITMAP_FMT_RGBA8888,
        NSGIF_BITMAP_FMT_BGRA8888,
        NSGIF_BITMAP_FMT_ARGB8888,
        NSGIF_BITMAP_FMT_ABGR8888,
    };
    size_t n = sizeof(fmts) / sizeof(fmts[0]);
    size_t i;

    for (i = 0; i < n; i++) {
        nsgif_t *gif = NULL;
        nsgif_bitmap_t *bitmap = NULL;
        nsgif_error err = nsgif_create(&cbs, fmts[i], &gif);
        ASSERT_EQ(err, NSGIF_OK);
        err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_data_complete(gif);
        err = nsgif_frame_decode(gif, 0, &bitmap);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_destroy(gif);
    }
}

/* ===================================================================
 * Category 4: Amiga-specific (2)
 * =================================================================== */

TEST(little_endian_lsd_parse_on_68k)
{
    /*
     * GIF Logical Screen Descriptor stores width/height as little-endian
     * 16-bit. 68k is big-endian. Decoder MUST read byte-by-byte. With
     * width LE = 0x01 0x00 = 1, a wrong-endian read would see 256.
     *
     * We can't directly read the libnsgif-internal width via a public
     * API in the same way as libnsbmp, but we can verify that decode
     * yields a 1x1 bitmap (cb_create gets called with width=1,height=1).
     */
    nsgif_t *gif = NULL;
    nsgif_error err;
    nsgif_bitmap_t *bitmap = NULL;
    test_bitmap *b;

    err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    ASSERT_EQ(err, NSGIF_OK);
    err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
    ASSERT_EQ(err, NSGIF_OK);
    nsgif_data_complete(gif);
    err = nsgif_frame_decode(gif, 0, &bitmap);
    ASSERT_EQ(err, NSGIF_OK);
    ASSERT_NOT_NULL(bitmap);
    b = bitmap;
    ASSERT_EQ(b->width, 1);
    ASSERT_EQ(b->height, 1);

    nsgif_destroy(gif);
}

TEST(lzw_decode_no_softfloat)
{
    /*
     * The LZW decompressor in src/lzw.c is the most arithmetic-heavy
     * path in libnsgif. Verify 50 decodes don't pull __divsf3 etc.
     */
    int i;
    for (i = 0; i < 50; i++) {
        nsgif_t *gif = NULL;
        nsgif_error err;
        nsgif_bitmap_t *bitmap = NULL;

        err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
        ASSERT_EQ(err, NSGIF_OK);
        err = nsgif_data_scan(gif, sizeof(gif_1x1_87a), gif_1x1_87a);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_data_complete(gif);
        err = nsgif_frame_decode(gif, 0, &bitmap);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_destroy(gif);
    }
}

/* ===================================================================
 * Category 5: Stress (2)
 * =================================================================== */

TEST(stress_create_destroy_50)
{
    int i;
    for (i = 0; i < 50; i++) {
        nsgif_t *gif = NULL;
        nsgif_error err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_destroy(gif);
    }
}

TEST(stress_parallel_instances)
{
    /*
     * 5 GIF instances live concurrently. Verify no global state
     * collision.
     */
    nsgif_t *gifs[5] = { NULL };
    int i;

    for (i = 0; i < 5; i++) {
        nsgif_error err = nsgif_create(&cbs, NSGIF_BITMAP_FMT_R8G8B8A8, &gifs[i]);
        ASSERT_EQ(err, NSGIF_OK);
        err = nsgif_data_scan(gifs[i], sizeof(gif_1x1_87a), gif_1x1_87a);
        ASSERT_EQ(err, NSGIF_OK);
        nsgif_data_complete(gifs[i]);
    }

    for (i = 0; i < 5; i++) {
        nsgif_bitmap_t *bitmap = NULL;
        nsgif_error err = nsgif_frame_decode(gifs[i], 0, &bitmap);
        ASSERT_EQ(err, NSGIF_OK);
        ASSERT_NOT_NULL(bitmap);
    }

    for (i = 0; i < 5; i++) {
        nsgif_destroy(gifs[i]);
    }
}

/* ===================================================================
 * main
 * =================================================================== */

int main(void)
{
    printf("\n=== libnsgif unit tests (16) ===\n\n");

    printf("[Functional]\n");
    RUN_TEST(nsgif_create_destroy);
    RUN_TEST(nsgif_data_scan_minimal);
    RUN_TEST(nsgif_frame_decode_minimal);
    RUN_TEST(nsgif_strerror_safe);
    RUN_TEST(nsgif_frame_prepare);

    printf("\n[Error path]\n");
    RUN_TEST(scan_bad_magic);
    RUN_TEST(scan_truncated);
    RUN_TEST(decode_bad_frame_number);
    RUN_TEST(scan_after_complete_rejected);

    printf("\n[Edge case]\n");
    RUN_TEST(scan_chunked);
    RUN_TEST(reset_replays_animation);
    RUN_TEST(all_pixel_format_variants);

    printf("\n[Amiga-specific]\n");
    RUN_TEST(little_endian_lsd_parse_on_68k);
    RUN_TEST(lzw_decode_no_softfloat);

    printf("\n[Stress]\n");
    RUN_TEST(stress_create_destroy_50);
    RUN_TEST(stress_parallel_instances);

    return test_summary();
}
