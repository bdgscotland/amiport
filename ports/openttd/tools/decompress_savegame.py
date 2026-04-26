#!/usr/bin/env python3
"""
decompress_savegame.py — convert OTTX/OTTZ OpenTTD savegames to OTTN (uncompressed).

OpenTTD's savegame header is 8 bytes:
    [0..3] = magic (OTTN / OTTZ / OTTX / OTTD)
    [4..7] = savegame format version (big-endian uint32)
    [8..] = payload (raw / zlib / LZMA depending on magic)

amiport openttd is built WITHOUT liblzma and WITHOUT zlib support (see toolchain file
`Freetype_FOUND FALSE`, `ZLIB_FOUND FALSE`, `LIBLZMA_FOUND FALSE`). Therefore any
OTTX savegame (including opntitle.dat from mainline OpenTTD releases) fails to load
with 'loader for lzma is not available', triggering the GenerateWorld fallback and
the multi-round NewGRF reload loop that blows up boot time.

Fix: decompress OTTX/OTTZ to OTTN on the host, deploy the OTTN file to the Amiga.
OTTN is a first-class format code in openttd's saveload, accepted unmodified.

Usage:
    ./decompress_savegame.py <input.dat> <output.dat>

Example (opntitle.dat):
    ./decompress_savegame.py \\
        ports/openttd/original/OpenTTD-13.4/build-dedicated/baseset/opntitle.dat \\
        /tmp/opntitle-ottn.dat

Then deploy with amigactl:
    /usr/bin/python3 -m amigactl --host <amiga-ip> put \\
        /tmp/opntitle-ottn.dat Programs:amiport-wip/OpenTTD/baseset/opntitle.dat
"""

import lzma
import sys
import zlib


def decompress(inpath: str, outpath: str) -> None:
    with open(inpath, "rb") as f:
        data = f.read()

    magic = data[:4]
    version = data[4:8]

    if magic == b"OTTN":
        print(f"{inpath} is already OTTN (no-compression). Copying as-is.")
        payload = data[8:]
    elif magic == b"OTTX":
        print(f"{inpath}: OTTX (LZMA), decompressing...")
        payload = lzma.decompress(data[8:])
    elif magic == b"OTTZ":
        print(f"{inpath}: OTTZ (zlib), decompressing...")
        payload = zlib.decompress(data[8:])
    elif magic == b"OTTD":
        print(f"{inpath}: OTTD (legacy uncompressed). Rewriting header as OTTN.")
        payload = data[8:]
    else:
        raise SystemExit(
            f"Unrecognised savegame magic: {magic!r}. "
            "Expected OTTN/OTTZ/OTTX/OTTD."
        )

    out = b"OTTN" + version + payload
    with open(outpath, "wb") as f:
        f.write(out)

    print(
        f"Wrote {outpath}: {len(out)} bytes "
        f"(magic=OTTN version={version.hex()} payload={len(payload)} bytes)"
    )


def main() -> None:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.dat> <output.dat>", file=sys.stderr)
        raise SystemExit(2)
    decompress(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
