# Amiga Development References

## Included in This Repo

- `posix-to-amiga-map.md` — Our master POSIX-to-AmigaOS mapping (in `.claude/skills/analyze-source/references/`)
- `amiga-api-reference.md` — Quick reference for key AmigaOS functions (in `.claude/skills/transform-source/references/`)
- `transformation-rules.md` — Source transformation rules (in `.claude/skills/transform-source/references/`)

## NDK (Native Development Kit)

The NDK contains the official include files, autodocs, and examples. Run the setup script to download:

```bash
make fetch-ndk
```

This downloads NDK 3.2 Release 4 from Hyperion Entertainment (freely distributed) and extracts the autodocs into `docs/references/ndk/`.

## Essential Books (External Links)

These are the canonical Amiga programming references, freely available on Archive.org:

### ROM Kernel Reference Manuals (RKMs)

| Book | Link | Priority |
|------|------|----------|
| **RKM: Libraries** (3rd Ed.) | [Archive.org](https://archive.org/details/amiga-rom-kernel-reference-manual-libraries-3rd-edition) | Essential — covers Exec, Intuition, DOS, Graphics |
| **RKM: Devices** (3rd Ed.) | [Archive.org](https://archive.org/details/amiga-rom-kernel-reference-manual-devices-3rd-edition) | Important — timer, serial, console devices |
| **RKM: Includes & Autodocs** (3rd Ed.) | [Archive.org](https://archive.org/details/amiga-rom-kernal-reference-includes-and-autodocs-3rd-edition) | Reference — function-by-function API docs |

### Other Key References

| Book | Link | Notes |
|------|------|-------|
| **AmigaDOS Manual** (3rd Ed.) | [Archive.org](https://archive.org/details/1991-baker-jesup-et-al-the-amigados-manual-3rd-ed) | DOS library, filesystem, CLI |
| **Hardware Reference Manual** (3rd Ed.) | [Archive.org](https://archive.org/details/amiga-hardware-reference-manual-3rd-edition) | Custom chips, DMA, copper |
| **Amiga Intern** (1992) | [Archive.org](https://archive.org/details/Amiga_Intern_1992_Abacus) | Deep technical internals |
| **Mapping the Amiga** | [Archive.org](https://archive.org/details/1993-thomson-randy-rhett-anderson) | Hardware register reference |

### Online References

| Resource | URL | Notes |
|----------|-----|-------|
| **ADCD 2.1 Online** | [amigadev.elowar.com](http://amigadev.elowar.com/) | Full developer CD contents, browsable HTML |
| **AmigaOS Wiki** | [wiki.amigaos.net](https://wiki.amigaos.net/) | AmigaOS 4.x focused but covers classic APIs |
| **Aminet** | [aminet.net](https://aminet.net/) | Software archive, includes dev tools |

### Not Freely Available

- **The Amiga Guru Book** by Ralph Babel — Copyright retained by author. Do not redistribute.
