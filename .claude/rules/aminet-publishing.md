Paths: ports/**/*.readme, ports/**/Makefile, scripts/publish-aminet.sh

# Aminet Publishing Rules — Deterministic Gates

Canonical reference: https://wiki.aminet.net/Uploading_instructions
**Always check the wiki before publishing. Rules may change.**

## Short Description (DESCRIPTION field in Makefile)

- **Maximum 40 characters.** This is enforced by `check-port-metadata.sh` (pre-commit) and `publish-aminet.sh` (publish time).
- **ASCII only.** No em-dashes, smart quotes, or any non-ASCII characters.
- The DESCRIPTION in the Makefile becomes the `Short:` field in the .readme.
- Count characters before writing. Verify after writing.

## Replaces: Field — NEVER HALLUCINATE

- **NEVER add `Replaces:` for first-time uploads.** If the package has never been on Aminet, there is nothing to replace. Aminet rejects uploads with `Replaces:` pointing to non-existent packages ("the file you specified to be replaced does not exist").
- **ONLY use `Replaces:`** when upgrading a package that currently exists on Aminet AND was previously uploaded by us.
- **Verify existence** on Aminet before adding. Use the `aminet-researcher` agent or check aminet.net directly.
- When in doubt, omit `Replaces:` entirely.

## .readme Format

- Required fields: `Short:`, `Uploader:`, `Type:`, `Architecture:`
- Max 78 characters per line
- LF line endings only (no CR+LF)
- Pure ASCII throughout

## Filename

- Max 30 characters including extension
- Lowercase preferred
- Only letters, digits, dots, underscores, hyphens
- Archive and .readme must have matching base names

## Validation Chain

These checks run automatically at different stages:

1. **Pre-commit** (`check-port-metadata.sh`): DESCRIPTION length, ASCII, Replaces: warning
2. **Publish time** (`publish-aminet.sh`): All of the above plus line lengths, filename length, LF endings
3. **Agent** (`aminet-publisher`): Must fetch wiki, verify all rules, show character counts in preview
