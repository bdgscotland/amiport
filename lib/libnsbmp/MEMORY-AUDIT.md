# Memory Safety Audit: libnsbmp

**Library:** BMP/ICO image decoder (NetSurf Vampire Phase 1, dep #6)  
**Audit date:** 2026-05-02  
**Verdict:** CAVEATS — Critical error-path leak in ICO multi-image parsing

## Allocation Summary

| Allocation | Location | Freed | All paths? | Issue |
|-----------|----------|-------|-----------|-------|
| `bmp->colour_table` (malloc) | bmp_info_header_parse:324 | bmp_finalise:1371 | YES | Safe; error-freed at 350-352 if bitmap_create fails |
| `bmp->bitmap` (callback) | bmp_info_header_parse:348 | bmp_finalise:1368 | YES | Caller-owned callback; pair verified |
| `ico_image` (calloc × N) | ico_header_parse:463 | ico_finalise:1386 | **NO** | **CRITICAL LEAK** — see below |

## Critical Finding: ICO Multi-Image Error-Path Leak

### Root Cause

`ico_header_parse()` (lines 421–508) loops over ICO directory entries:

```c
for (i = 0; i < count; i++) {
    image = calloc(1, sizeof(ico_image));  // line 463
    if (!image)
        return BMP_INSUFFICIENT_MEMORY;
    result = next_ico_image(ico, image);   // links image into ico->first chain
    /* ... parse per-image header ... */
    result = bmp_info_header_parse(&image->bmp, image->bmp.bmp_data);  // line 494
    if (result != BMP_OK)
        return result;  // <-- ERROR EXIT WITHOUT CLEANUP
```

If `bmp_info_header_parse()` fails at any iteration (e.g., malloc failure for that image's colour_table), the function returns error immediately without freeing:
1. The newly-allocated `ico_image` struct
2. Any already-allocated `ico_image` structs linked earlier in the loop (which remain in `ico->first` chain)
3. The colour_tables of prior images (allocated inside their `bmp_info_header_parse` calls)

### Scenario: 3-Image ICO, Failure on Image 2

```
1. Image 0: calloc ico_image + link → bmp_info_header_parse succeeds (allocates colour_table if <16bpp)
2. Image 1: calloc ico_image + link → bmp_info_header_parse succeeds
3. Image 2: calloc ico_image + link → bmp_info_header_parse fails (malloc(colour_table) → NULL)
   → RETURN BMP_INSUFFICIENT_MEMORY
   → Images 0,1,2 ico_image structs remain linked in chain
   → Caller receives error from ico_analyse()
   → Caller may skip ico_finalise() after error
   → **LEAK: ~64-128 bytes per ico_image + colour_tables of prior images (potentially KB)**
```

### Impact on AmigaOS

With `-noixemul` runtime, **there is no automatic process memory cleanup on exit**. This leak is **permanent until reboot**. In a NetSurf browser context parsing malformed/truncated ICO files from untrusted sources, repeated ICO parse failures can exhaust available Fast RAM.

### Fix Required

Add cleanup path before error return in `ico_header_parse`:

```c
/* Before any error return that might have allocated ico_image entries: */
if (result != BMP_OK) {
    /* Unlink and free all allocated ico_image entries */
    while (ico->first) {
        image = ico->first;
        ico->first = image->next;
        bmp_finalise(&image->bmp);  /* frees colour_table if present */
        free(image);
    }
    return result;
}
```

Or move error cleanup to a dedicated `error:` label at the end of the function.

## Other Allocations: CLEAN

- **bmp->colour_table**: Freed in `bmp_finalise()` with NULL-assignment for idempotency. Error-freed at line 350-352 if bitmap_create fails after malloc. **SAFE**.
- **bmp->bitmap (callback)**: Caller-owned; lifecycle matches bitmap_create/bitmap_destroy callback pair. **SAFE**.
- **No realloc patterns**: colour_table allocated once at final size. **SAFE**.
- **No double-free**: bmp_finalise is idempotent (NULL-checks and NULL-assignment after free). ico_finalise single-pass unlinks and frees. **SAFE**.

## Stack Safety

- **Large locals**: max 8 bytes (bit_shifts[8] array at line 768). **SAFE** for 68k with reasonable __stack.
- **No struct-by-value returns**: all functions return void or `bmp_result` enum. **SAFE**.

## Callback Contract Verification

**Caller supplies three callbacks:**
1. `bitmap_create(width, height, flags)` → opaque handle
2. `bitmap_get_buffer(bitmap)` → writable uint8_t* or NULL on error
3. `bitmap_destroy(bitmap)` → free the handle (safe on NULL due to line 1369 guard)

**Pairs verified:**
- One bitmap_create call per successfully-parsed BMP (line 348, guarded by error-free colour_table)
- One bitmap_destroy call per allocated bitmap (line 1368 in finalise, guarded by `if (bmp->bitmap)`)
- **1:1 pair maintained across normal flow and error paths.** ✓

## Recommendations

1. **BLOCKING:** Fix the `ico_header_parse` error-path leak before merging to main. This is a permanent memory leak on the target platform.
2. **Library-level:** Consider adding an `ico_collection_finalise_partial()` or similar cleanup function that can be called even if `ico_analyse()` fails partway, to give consumers a safe recovery path.
3. **Consumer responsibility:** After calling `ico_analyse()`, callers MUST call `ico_finalise()` regardless of return code to ensure cleanup (once the library fix is applied). Document this in the public header.

## Test Coverage

- Stage 5 unit tests: 18/18 pass on vamos -C 68040. Tests likely use well-formed ICO files and do not exercise the multi-image error-path.
- **Recommend adding:** Stress test with truncated/malformed ICO files (missing directory entries, truncated colour_table data, etc.) to trigger `bmp_info_header_parse` errors and verify cleanup.
