# Apollo SAGA Sprite Hardware Reference

**Source:** Apollo Core SAGA Technical Reference Manual, Chapter 1: Sprite Hardware (revision 1.0). 19-page PDF, author "john", PDF metadata 2020-12-17. Obtained 2026-05-02.

**Status:** Authoritative for SAGA / AGA+ sprite hardware on Apollo Core (Vampire V2/V4/A6000). Useful for any amiport game port that wants to use Apollo's enhanced sprite hardware (Julius, OpenTTD, future SDL games).

## Sprite evolution overview

Three generations of sprite hardware available on Apollo Core:

1. **OCS/ECS (classic)**: 8 sprite DMA channels, 16 px wide, 3 colors per pixel + transparent, sprites can be combined ("attached") for 15 colors + transparent.
2. **AGA enhancements**:
   - Sprites can be 16 / 32 / 64 px wide
   - LORES / HIRES / SHIRES resolutions independent of bitplane
   - Hardware scan-doubling (display 31 kHz)
   - Sub-pixel positioning in low-res
   - Palette bank switching (sprites use colors independent of bitplanes)
3. **SAGA (AGA+)**:
   - 16 sprite DMA channels (vs 8) — twice the on-screen sprite count
   - **No DMA bandwidth loss** (sprites don't steal cycles from bitplanes)
   - **Sprite data can reside in Fast RAM** (no longer Chip-only)
   - Horizontal repetition without copper tricks
   - Indirect data mode for fast sprite-data switching
   - Each sprite has its own 256-color palette bank
   - Each sprite uses its own 16-color set from that bank (no need to attach for 16 colors)
   - 24-bit RGB palette entries
   - Single-instruction 32-bit palette move via new copper command
   - Controlled by a new bit in `FMODE` (`$dff1fc`) for backwards compatibility

## Enabling AGA+ sprite operation

In the **FMODE** register (`$dff1fc`):

| Bit | Name | Description |
|-----|------|-------------|
| 15 | SSCAN2 | Global sprite scan doubling |
| 14 | BSCAN2 | Bitplane scan doubling |
| 4 | SAGA Enable | Enables 32-bit copper, enhanced sprites, etc. |
| 3 | SPAGEM | Sprite page mode (double CAS) |
| 2 | SPR32 | Sprite 32-bit wide mode |
| 1 | BPAGEM | Bitplane page mode (double CAS) |
| 0 | BPL32 | Bitplane 32-bit wide mode |

To enable AGA+ sprites: set **SAGA Enable + SPAGEM + SPR32** (all three). All sprites in AGA+ mode are 64-bit aligned and 64-pixels wide.

## Sprite data structure

Generic structure (16-bit words):

```
Memory N    : Sprite Control Word 1 (Vertical/Horizontal Start Position)
Memory N+2  : Sprite Control Word 2 (Vertical Stop Position + control bits)
Memory N+4  : Sprite Binary Image Data Low Word 1   ─┐
Memory N+6  : Sprite Binary Image Data High Word 1  ─┘ Row 1 colour data
Memory N+8  : Sprite Binary Image Data Low Word 2   ─┐
Memory N+10 : Sprite Binary Image Data High Word 2  ─┘ Row 2 colour data
...
End-Of-Data : two zero-words to mark end (or new control words for re-use)
```

Alignment requirements:
- 16-pixel sprite: 16-bit boundary
- 32-pixel sprite: 32-bit boundary
- 64-pixel sprite: 64-bit boundary

**Apollo Core relaxes the Chip RAM requirement** — sprite data can live in Fast RAM. This is a major perf win because Fast RAM is much faster than Chip RAM on Vampire.

## Control word layouts

**SPRxPOS (Position):**

| Bits | Function |
|------|----------|
| 15-08 | Low 8 bits of VSTART |
| 07-00 | High 8 bits of HSTART |

**SPRxCTL (Control):**

| Bits | Function |
|------|----------|
| 15-08 | Low 8 bits of VSTOP |
| 07 | ATT (sprite attach control bit, odd-numbered sprites only) |
| 06-03 | Unused (must be zero) |
| 02 | VSTART high bit |
| 01 | VSTOP high bit |
| 00 | HSTART low bit |

## AGA+ extended control word (64-bit)

AGA+ uses 32-pixel-wide sprites with 4-bit indices (16 colors directly without attaching). Control words are extended to 64 bits with new bits at the bottom:

```
[Original 16-bit Control Word | Padding | Padding | New Bits]
                                                    ├── SAGA Mode (0)
                                                    ├── Horizontal Repeat (1)
                                                    └── Palette Select (7..4)
```

Sprite image data is no longer attached — all 4 bits per pixel directly in the data. Image data is referenced indirectly via a pointer in the control structure (allows animated sprites without rewriting control words):

```asm
    CNOP 0,8

Sprite0_Control:
    dc.l    $50600000, $00000021
    dc.l    $60000000, PlayerImage

PlayerImage:
    dc.l    $ffffffff, $00000000, $ffffffff, $00000000
```

The Sprite Image data above yields binary value `1010` for each of the 32 pixels in that row.

## Color palette banks (SAGA)

SAGA provides four separate 256-entry palettes:
- **Palette A** — AGA classic, used in OCS/ECS/AGA mode
- **Palette B** — Sprite-specific (AGA+ mode)
- **Palette C** — Chunky display modes
- **Palette D** — PiP (Picture-in-Picture) CLUT

In AGA+ mode, sprites use Palette B exclusively, decoupled from bitplane palette:
- Sprite 0 → entries 0..15
- Sprite 1 → entries 16..31
- Sprite 2 → entries 32..47
- ... Sprite 15 → entries 240..255

## Fast 32-bit palette registers

New per-bank 32-bit color registers allow setting full 24-bit palette entry in a single move (CPU OR copper):

| Address | Bank |
|---------|------|
| `$DFF380/382` | Planar Palette A |
| `$DFF384/386` | Sprite Palette B |
| `$DFF388/38A` | Chunky Palette C |
| `$DFF38C/38E` | PiP Palette D |

Layout (32 bits):
- Bits 31-24: Colour Number
- Bits 23-16: 8-bit Red
- Bits 15-08: 8-bit Green
- Bits 07-00: 8-bit Blue

```asm
move.l  #$00FF0000,$DFF384   ; set sprite color 0 = RED
move.l  #$01FFFF00,$DFF384   ; set sprite color 1 = YELLOW

; Or via Copper Move Long (when FMODE SAGA Enable is set):
dc.w    $8384,$00FF,$0000     ; set sprite color 0 = RED
```

## Sprite registers

Original 8 sprites (DFF120 - DFF146) plus AGA+ extended 8 sprites (DFF320 - DFF346):

| Address | Name | R/W | Description |
|---------|------|-----|-------------|
| DFF120 | SPR0PTH | W | Sprite 0 pointer (high 5 bits) |
| DFF122 | SPR0PTL | W | Sprite 0 pointer (low 15 bits) |
| ... | ... | ... | ... (SPR1-7 pointers) |
| DFF140 | SPR0POS | W | Sprite 0 Position |
| DFF142 | SPR0CTL | W | Sprite 0 Control |
| DFF144 | SPR0DATA | W | Sprite 0 Data A |
| DFF146 | SPR0DATB | W | Sprite 0 Data B |
| DFF320-DFF33E | SPR8-15PT(H/L) | W | Sprite 8-15 pointers |
| DFF340-DFF346 | SPR8POS/CTL/DATA/DATB | W | Sprite 8 control |
| ... | ... | ... | ... |

Pointer registers contain the 20-bit memory address (2 MB Chip Memory limit for classic) OR full 32-bit address for AGA+.

## Sprite priority

Sprite 0 is in front; Sprite N (15 in AGA+) is at the back. Lower number = higher priority. Both AGA (8 sprite) and AGA+ (16 sprite) modes follow this fixed priority.

## Attaching sprites (OCS/ECS/AGA only)

Pairs of sprites can be combined for 15 colors + transparent:
- Sprite 1 + 0
- Sprite 3 + 2
- Sprite 5 + 4
- Sprite 7 + 6

Set ATTACH bit (bit 7) in the second sprite's control word (the odd-numbered one). Data from both sprites is combined bitwise into a 4-bit value (0-15) mapping to color registers 16-31. **Not needed in AGA+** since 16 colors are always available per sprite.

## Reusing sprite DMA channels

Each DMA channel can produce more than one independent sprite, vertically stacked. The constraint:
- **OCS/ECS/AGA**: at least one video line must separate the bottom of one usage and the top of the next
- **AGA+**: this restriction is removed via "double-fetch" on the last row — re-use can occur on the immediately adjacent row

Construction: place a complete new sprite control structure where the end-of-data zero-words would normally go.

## Manual mode (CPU-driven)

Alternative to DMA mode: load `SPRxDATB` then `SPRxDATA` (in that order). Writing `SPRxDATA` "arms" the sprite. Data is displayed on every row until more is loaded. Disable by writing `SPRxCTL`. `SPRxPOS` writes can move the sprite at any time.

Almost always best to use automatic DMA mode unless doing copper-style tricks.

## Apollo-specific perf advantages summary

For amiport game ports targeting Vampire:
- **16 sprites** (double the classic 8) with no DMA contention
- **Sprite data in Fast RAM** — much faster than Chip RAM access
- **Per-sprite 16-color palette** — no need to combine sprites for color depth
- **24-bit RGB color** — full Picasso96-quality sprites
- **Indirect data mode** — animated sprites without copper rewriting
- **No DMA bandwidth loss** — sprites don't steal cycles from bitplanes

For text-rendering / NetSurf font compositor (Phase 1): NOT directly relevant. Text rendering uses the framebuffer (RTG / chunky) path, not sprite hardware. SAGA chunky display modes (in a different SAGA TRM chapter we don't have yet) are the relevant section for NetSurf.

## Additional reading

- [codetapper.com/amiga/sprite-tricks/](https://codetapper.com/amiga/sprite-tricks/) — clever sprite tricks in classic Amiga games

## Cross-references

- `docs/references/ammx/instruction-reference.md` — Apollo PRM AMMX (different SIMD path, not sprite hardware)
- `docs/references/vampire-sdk/headers.md` — Vampire SDK init pattern (vampire.resource is separate from sprite hardware)
- Sprite hardware is OCS/ECS/AGA legacy + SAGA enhancements; for chunky display modes (RTG-style framebuffer), the SAGA TRM has separate chapters not yet captured

## Discovery context

PDF obtained from user's downloads 2026-05-02. Author: "john" (per PDF metadata). Part of the Apollo Core SAGA TRM (revision 1.0). This is chapter 1 only; full TRM has additional chapters covering chunky display modes, audio (Pamela), Maggie 3D, etc. — those would be valuable future ingestions.
