# amiport — Ported Software Catalog

Programs ported to AmigaOS 3.x using the amiport pipeline.

| Port | Version | Description | Category | Original | Status | Aminet |
|------|---------|-------------|----------|----------|--------|--------|
| [cal](ports/cal/) | 1.0 | Unix-style calendar display | CLI | OpenBSD cal v1.32 (BSD 3-Clause) | Built & tested | Not submitted |
| [diff](ports/diff/) | 1.0 | File comparison utility | CLI | OpenBSD diff v1.95 (ISC + BSD 3-Clause) | Built & tested | Not submitted |

## Aminet Publication Tracking

| Port | Submitted | Aminet URL | Last Checked |
|------|-----------|-----------|-------------|
| cal | — | — | — |
| diff | — | — | — |

Run `make check-aminet` to verify publication status.

## Shim Functions Coverage

Each port exercises different parts of the posix-shim library:

| Port | Shim Functions Used |
|------|-------------------|
| cal | `getopt`, `err/errx`, `strtonum` |
| diff | `getopt`, `err/errx/warn/warnc`, `stat`, `opendir/readdir/closedir`, `scandir/alphasort`, `fnmatch`, `tmpfile` |

## How to Build

```bash
make build TARGET=ports/cal     # Build a specific port
make build-ports                # Build all ports
make test TARGET=ports/cal      # Test in vamos
make list-ports                 # Show status of all ports
```

## How to Add a Port

Use the `/port-project` skill — it handles the full pipeline:
```
/port-project /path/to/source
```

See [docs/porting-guide.md](docs/porting-guide.md) for the manual process.
