/*
 * test_freetype.c -- Unit tests for lib/freetype (FreeType 2.13.3
 *                   SDL_ttf-only subset on AmigaOS 68k)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#include "test_framework.h"

long __stack = 262144;

#define TEST_FONT_PATH "test-font.ttf"
#define TEST_FONT_CORRUPT_PATH "test-font-corrupt.dat"

static FT_Library g_lib;
static FT_Face g_face;

static int init_lib(void)
{
    return FT_Init_FreeType(&g_lib);
}

static int open_face(void)
{
    return FT_New_Face(g_lib, TEST_FONT_PATH, 0, &g_face);
}

static void close_face(void)
{
    if (g_face) { FT_Done_Face(g_face); g_face = NULL; }
}

static void done_lib(void)
{
    if (g_lib) { FT_Done_FreeType(g_lib); g_lib = NULL; }
}

/* --- Functional: pure arithmetic --- */

TEST(mulfix_identity)
{
    FT_Fixed result = FT_MulFix(0x10000L, 0x10000L);
    ASSERT_EQ(result, 0x10000L);
}

TEST(mulfix_known_values)
{
    ASSERT_EQ(FT_MulFix(0, 0x10000L), 0);
    ASSERT_EQ(FT_MulFix(0x10000L, 0), 0);
    ASSERT_EQ(FT_MulFix(0x10000L, 0x8000L), 0x8000L);
    ASSERT_EQ(FT_MulFix(0x20000L, 0x10000L), 0x20000L);
    ASSERT_EQ(FT_MulFix(-0x10000L, 0x10000L), -0x10000L);
}

/* --- Functional: lifecycle --- */

TEST(library_init_done)
{
    FT_Library lib = NULL;
    FT_Error err = FT_Init_FreeType(&lib);
    ASSERT_EQ(err, 0);
    ASSERT_NOT_NULL(lib);
    err = FT_Done_FreeType(lib);
    ASSERT_EQ(err, 0);
}

TEST(library_double_init)
{
    FT_Library lib1 = NULL, lib2 = NULL;
    ASSERT_EQ(FT_Init_FreeType(&lib1), 0);
    ASSERT_EQ(FT_Init_FreeType(&lib2), 0);
    ASSERT_NOT_NULL(lib1);
    ASSERT_NOT_NULL(lib2);
    ASSERT_EQ(FT_Done_FreeType(lib2), 0);
    ASSERT_EQ(FT_Done_FreeType(lib1), 0);
}

/* --- Functional: face creation --- */

TEST(face_open_valid)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_NOT_NULL(g_face);
    close_face();
    done_lib();
}

TEST(face_properties)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT(g_face->num_glyphs > 0);
    ASSERT_NOT_NULL(g_face->family_name);
    ASSERT(FT_IS_SCALABLE(g_face));
    ASSERT(g_face->num_charmaps > 0);
    close_face();
    done_lib();
}

/* --- Error path --- */

TEST(face_open_nonexistent)
{
    FT_Face face = NULL;
    ASSERT_EQ(init_lib(), 0);
    FT_Error err = FT_New_Face(g_lib, "nonexistent-9999.ttf", 0, &face);
    ASSERT(err != 0);
    ASSERT_NULL(face);
    done_lib();
}

TEST(face_open_corrupt)
{
    FT_Face face = NULL;
    ASSERT_EQ(init_lib(), 0);
    FT_Error err = FT_New_Face(g_lib, TEST_FONT_CORRUPT_PATH, 0, &face);
    ASSERT(err != 0);
    ASSERT_NULL(face);
    done_lib();
}

TEST(face_reopen_after_done)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    close_face();
    ASSERT_EQ(open_face(), 0);
    ASSERT_NOT_NULL(g_face);
    close_face();
    done_lib();
}

/* --- Functional: sizing --- */

TEST(pixel_sizes_valid)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    ASSERT_EQ(g_face->size->metrics.y_ppem, 16);
    close_face();
    done_lib();
}

TEST(char_size_valid)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Char_Size(g_face, 0, 12 * 64, 72, 72), 0);
    close_face();
    done_lib();
}

TEST(pixel_sizes_zero)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    /* FreeType accepts (0,0) — it uses defaults internally rather than
       rejecting. This test verifies it doesn't crash. */
    FT_Set_Pixel_Sizes(g_face, 0, 0);
    ASSERT(1);
    close_face();
    done_lib();
}

/* --- Functional: charmap --- */

TEST(char_index_ascii)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT(idx > 0);
    close_face();
    done_lib();
}

TEST(char_index_unmapped)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 0xFFFF);
    ASSERT_EQ(idx, 0);
    close_face();
    done_lib();
}

/* --- Functional: glyph loading --- */

TEST(load_glyph_default)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT(idx > 0);
    ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
    ASSERT(g_face->glyph->metrics.width > 0);
    ASSERT(g_face->glyph->metrics.height > 0);
    close_face();
    done_lib();
}

TEST(load_glyph_out_of_range)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_Error err = FT_Load_Glyph(g_face, (FT_UInt)(g_face->num_glyphs + 100),
                                  FT_LOAD_DEFAULT);
    ASSERT(err != 0);
    close_face();
    done_lib();
}

/* --- Functional: rendering --- */

TEST(render_glyph_normal)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
    ASSERT_EQ(FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL), 0);
    ASSERT_EQ(g_face->glyph->format, FT_GLYPH_FORMAT_BITMAP);
    close_face();
    done_lib();
}

TEST(render_glyph_bitmap_content)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
    ASSERT_EQ(FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL), 0);
    ASSERT(g_face->glyph->bitmap.width > 0);
    ASSERT(g_face->glyph->bitmap.rows > 0);
    ASSERT_NOT_NULL(g_face->glyph->bitmap.buffer);
    ASSERT(g_face->glyph->bitmap.pitch > 0);
    close_face();
    done_lib();
}

/* --- Functional: kerning --- */

TEST(get_kerning_valid)
{
    FT_Vector kern;
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx_a = FT_Get_Char_Index(g_face, 'A');
    FT_UInt idx_v = FT_Get_Char_Index(g_face, 'V');
    FT_Error err = FT_Get_Kerning(g_face, idx_a, idx_v,
                                   FT_KERNING_DEFAULT, &kern);
    ASSERT_EQ(err, 0);
    if (FT_HAS_KERNING(g_face)) {
        ASSERT(kern.x != 0);
    }
    close_face();
    done_lib();
}

TEST(get_kerning_same_glyph)
{
    FT_Vector kern;
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx_a = FT_Get_Char_Index(g_face, 'A');
    ASSERT_EQ(FT_Get_Kerning(g_face, idx_a, idx_a,
                              FT_KERNING_DEFAULT, &kern), 0);
    ASSERT_EQ(kern.x, 0);
    ASSERT_EQ(kern.y, 0);
    close_face();
    done_lib();
}

/* --- Functional: outline transform --- */

TEST(outline_transform_identity)
{
    FT_Matrix matrix;
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
    ASSERT_EQ(g_face->glyph->format, FT_GLYPH_FORMAT_OUTLINE);
    FT_Pos orig_x = g_face->glyph->outline.points[0].x;
    matrix.xx = 0x10000L; matrix.xy = 0;
    matrix.yx = 0; matrix.yy = 0x10000L;
    FT_Outline_Transform(&g_face->glyph->outline, &matrix);
    ASSERT_EQ(g_face->glyph->outline.points[0].x, orig_x);
    close_face();
    done_lib();
}

/* --- Amiga-specific --- */

TEST(amiga_big_endian_sfnt)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT(g_face->units_per_EM >= 256);
    ASSERT(g_face->units_per_EM <= 4096);
    close_face();
    done_lib();
}

TEST(amiga_bitmap_alignment)
{
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    FT_UInt idx = FT_Get_Char_Index(g_face, 'A');
    ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
    ASSERT_EQ(FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL), 0);
    ASSERT(((unsigned long)g_face->glyph->bitmap.buffer & 1) == 0);
    close_face();
    done_lib();
}

/* --- Stress --- */

TEST(stress_az_load_render)
{
    unsigned long ch;
    ASSERT_EQ(init_lib(), 0);
    ASSERT_EQ(open_face(), 0);
    ASSERT_EQ(FT_Set_Pixel_Sizes(g_face, 0, 16), 0);
    for (ch = 'A'; ch <= 'Z'; ch++) {
        FT_UInt idx = FT_Get_Char_Index(g_face, ch);
        ASSERT(idx > 0);
        ASSERT_EQ(FT_Load_Glyph(g_face, idx, FT_LOAD_DEFAULT), 0);
        ASSERT_EQ(FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL), 0);
        ASSERT_NOT_NULL(g_face->glyph->bitmap.buffer);
    }
    close_face();
    done_lib();
}

TEST(stress_repeated_open_close)
{
    int i;
    ASSERT_EQ(init_lib(), 0);
    for (i = 0; i < 10; i++) {
        ASSERT_EQ(open_face(), 0);
        ASSERT_NOT_NULL(g_face);
        close_face();
    }
    done_lib();
}

TEST(stress_mulfix_range)
{
    static const int sizes[] = {8, 12, 16, 24, 32, 48, 64};
    FT_Fixed prev = 0;
    int i;
    for (i = 0; i < 7; i++) {
        FT_Fixed scale = (FT_Fixed)sizes[i] * 64;
        FT_Fixed result = FT_MulFix(2048L * 0x10000L, scale);
        ASSERT(result > 0);
        ASSERT(result > prev);
        prev = result;
    }
}

/* --- Main --- */

int main(void)
{
    printf("FreeType 2.13.3 unit tests\n\n");

    RUN_TEST(mulfix_identity);
    RUN_TEST(mulfix_known_values);
    RUN_TEST(library_init_done);
    RUN_TEST(library_double_init);
    RUN_TEST(face_open_valid);
    RUN_TEST(face_properties);
    RUN_TEST(face_open_nonexistent);
    RUN_TEST(face_open_corrupt);
    RUN_TEST(face_reopen_after_done);
    RUN_TEST(pixel_sizes_valid);
    RUN_TEST(char_size_valid);
    RUN_TEST(pixel_sizes_zero);
    RUN_TEST(char_index_ascii);
    RUN_TEST(char_index_unmapped);
    RUN_TEST(load_glyph_default);
    RUN_TEST(load_glyph_out_of_range);
    RUN_TEST(render_glyph_normal);
    RUN_TEST(render_glyph_bitmap_content);
    RUN_TEST(get_kerning_valid);
    RUN_TEST(get_kerning_same_glyph);
    RUN_TEST(outline_transform_identity);
    RUN_TEST(amiga_big_endian_sfnt);
    RUN_TEST(amiga_bitmap_alignment);
    RUN_TEST(stress_az_load_render);
    RUN_TEST(stress_repeated_open_close);
    RUN_TEST(stress_mulfix_range);

    return test_summary();
}
