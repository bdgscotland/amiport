/* FreeType 2.13.3 visual render demo -- prove the -O1 hot-path build
 * actually rasterizes visible glyphs (not just "buffer != NULL").
 *
 * Renders 'A' at 20px and dumps the 256-level AA bitmap as ASCII art.
 * If the rasterizer is broken, we get a blank page or garbage. If it
 * works, we see the letter A.
 *
 * Run: vamos -s 256 -m 4096 ./render_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

int main(void)
{
    FT_Library lib;
    FT_Face face;
    FT_Error err;
    FT_GlyphSlot g;
    unsigned int r, c;
    unsigned long nonzero = 0;
    unsigned long max_pixel = 0;
    unsigned long sum = 0;
    static const char ramp[] = " .:-=+*#%@";

    if ((err = FT_Init_FreeType(&lib)) != 0) {
        fprintf(stderr, "FT_Init_FreeType: %d\n", err);
        return 10;
    }
    if ((err = FT_New_Face(lib, "test-font.ttf", 0, &face)) != 0) {
        fprintf(stderr, "FT_New_Face: %d\n", err);
        return 10;
    }
    if ((err = FT_Set_Pixel_Sizes(face, 0, 20)) != 0) {
        fprintf(stderr, "FT_Set_Pixel_Sizes: %d\n", err);
        return 10;
    }
    if ((err = FT_Load_Char(face, 'A', FT_LOAD_RENDER)) != 0) {
        fprintf(stderr, "FT_Load_Char: %d\n", err);
        return 10;
    }

    g = face->glyph;
    printf("Glyph 'A' rendered: %dx%d (pitch=%d)\n",
           (int)g->bitmap.width, (int)g->bitmap.rows, (int)g->bitmap.pitch);

    for (r = 0; r < g->bitmap.rows; r++) {
        for (c = 0; c < g->bitmap.width; c++) {
            unsigned char px = g->bitmap.buffer[r * g->bitmap.pitch + c];
            int idx = px * (int)(sizeof(ramp) - 1) / 256;
            putchar(ramp[idx]);
            if (px != 0) nonzero++;
            if (px > max_pixel) max_pixel = px;
            sum += px;
        }
        putchar('\n');
    }

    printf("Pixels: %lu nonzero, max=%lu, sum=%lu\n",
           nonzero, max_pixel, sum);

    FT_Done_Face(face);
    FT_Done_FreeType(lib);

    if (nonzero == 0 || max_pixel == 0) {
        fprintf(stderr, "FAIL: rasterizer produced blank bitmap\n");
        return 10;
    }
    if (max_pixel < 128) {
        fprintf(stderr, "FAIL: rasterizer max pixel %lu < 128 (expected ~255)\n",
                max_pixel);
        return 10;
    }
    printf("OK -- glyph rendered with visible content\n");
    return 0;
}
