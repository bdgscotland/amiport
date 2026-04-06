#!/bin/bash
# build-machine-lhas.sh — Build machine-format LHA packages for amiget
#
# For each published stable port with a compiled binary, this script:
#   1. Creates a temp directory with C/<name> (the binary)
#   2. Runs `lha a <name>-<version>-machine.lha C/<name>` via Docker
#   3. Copies the machine LHA to site/packages/
#   4. Computes SHA-256 and updates machine_sha256 in the package JSON
#
# Machine LHA naming: <name>-<version>-machine.lha (no revision suffix)
# The C/ prefix means amiget extracts to SYS:C/<name> = C:<name>
#
# Usage:
#   bash scripts/build-machine-lhas.sh              # All stable packages
#   bash scripts/build-machine-lhas.sh grep jq vim  # Specific packages only
#
# Requirements:
#   - Docker (for lha via amigadev/crosstools)
#   - python3 (for JSON updates and SHA-256)
#   - Compiled binaries in ports/<name>/<name>

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PACKAGES_DIR="$PROJECT_ROOT/site/packages"
DATA_DIR="$PROJECT_ROOT/site/data/packages"
PORTS_DIR="$PROJECT_ROOT/ports"

# Docker image with lha
DOCKER_IMAGE="amigadev/crosstools:m68k-amigaos"

# Temporary working directory
TMPDIR_BASE="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_BASE"' EXIT

log()  { echo "[build-machine-lhas] $*"; }
warn() { echo "[build-machine-lhas] WARN: $*" >&2; }
err()  { echo "[build-machine-lhas] ERROR: $*" >&2; exit 1; }

# Verify Docker is available
if ! docker info >/dev/null 2>&1; then
    err "Docker is not running. Start Docker and try again."
fi

# Verify lha is available via Docker
if ! docker run --rm "$DOCKER_IMAGE" lha --version >/dev/null 2>&1; then
    err "lha not found in Docker image $DOCKER_IMAGE"
fi

# Get list of package names to process
if [ $# -gt 0 ]; then
    # Specific packages requested
    REQUESTED=("$@")
else
    REQUESTED=()
fi

BUILT=0
SKIPPED=0
FAILED=0

for json_file in "$DATA_DIR"/*.json; do
    pkg_name="$(python3 -c "import json,sys; d=json.load(open('$json_file')); print(d['name'])")"
    pkg_version="$(python3 -c "import json,sys; d=json.load(open('$json_file')); print(d['version'])")"
    pkg_status="$(python3 -c "import json,sys; d=json.load(open('$json_file')); print(d.get('status','stable'))")"

    # If specific packages requested, skip others
    if [ ${#REQUESTED[@]} -gt 0 ]; then
        found=0
        for req in "${REQUESTED[@]}"; do
            if [ "$req" = "$pkg_name" ]; then
                found=1
                break
            fi
        done
        if [ $found -eq 0 ]; then
            continue
        fi
    fi

    # Only process stable packages
    if [ "$pkg_status" != "stable" ]; then
        log "SKIP $pkg_name ($pkg_status — not stable)"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Check binary exists
    BINARY="$PORTS_DIR/$pkg_name/$pkg_name"
    if [ ! -f "$BINARY" ]; then
        warn "$pkg_name: no binary at $BINARY — skipping"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # Machine LHA filename uses base version only (no revision suffix)
    # The revision suffix is for Aminet bundles; machine LHAs track upstream version
    MACHINE_LHA_NAME="${pkg_name}-${pkg_version}-machine.lha"
    MACHINE_LHA_DEST="$PACKAGES_DIR/$MACHINE_LHA_NAME"

    log "Building $MACHINE_LHA_NAME ..."

    # Build in a fresh temp directory
    WORK="$TMPDIR_BASE/$pkg_name"
    mkdir -p "$WORK/C"
    cp "$BINARY" "$WORK/C/$pkg_name"

    # Strip debug info if present (keep binary small)
    # Use Docker's m68k strip if available, otherwise skip silently
    docker run --rm \
        -v "$WORK:/work" \
        "$DOCKER_IMAGE" \
        sh -c "m68k-amigaos-strip /work/C/$pkg_name 2>/dev/null || true"

    # Build the LHA archive via Docker
    # lha must run from inside the work dir so the archive contains C/<name>
    # (not an absolute path)
    if ! docker run --rm \
        -v "$WORK:/work" \
        -w /work \
        "$DOCKER_IMAGE" \
        lha a "/work/$MACHINE_LHA_NAME" "C/$pkg_name" >/dev/null 2>&1; then
        warn "$pkg_name: lha build failed"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Verify the archive was created
    if [ ! -f "$WORK/$MACHINE_LHA_NAME" ]; then
        warn "$pkg_name: lha produced no output file"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Compute SHA-256 (lowercase hex) — portable: sha256sum (Linux) or shasum (macOS)
    if command -v sha256sum >/dev/null 2>&1; then
        SHA256="$(sha256sum "$WORK/$MACHINE_LHA_NAME" | awk '{print $1}')"
    else
        SHA256="$(shasum -a 256 "$WORK/$MACHINE_LHA_NAME" | awk '{print $1}')"
    fi

    # Copy to site/packages/
    cp "$WORK/$MACHINE_LHA_NAME" "$MACHINE_LHA_DEST"
    log "  -> $MACHINE_LHA_DEST (SHA-256: $SHA256)"

    # Update machine_sha256 in the package JSON
    python3 - "$json_file" "$SHA256" <<'PYEOF'
import json, sys

json_path = sys.argv[1]
sha256 = sys.argv[2]

with open(json_path) as f:
    data = json.load(f)

data['machine_sha256'] = sha256

with open(json_path, 'w') as f:
    json.dump(data, f, indent=4, ensure_ascii=False)
    f.write('\n')

print(f"  -> updated {json_path}")
PYEOF

    BUILT=$((BUILT + 1))
done

echo ""
log "Done: $BUILT built, $SKIPPED skipped, $FAILED failed"

if [ $BUILT -gt 0 ]; then
    echo ""
    log "Next steps:"
    log "  1. Regenerate packages.json manifest:"
    log "     python3 -c \"import json,glob; pkgs=[json.load(open(f)) for f in sorted(glob.glob('site/data/packages/*.json'))]; json.dump({'version':1,'packages':pkgs}, open('site/api/v1/packages.json','w'), indent=2)\""
    log "  2. Deploy to Dreamhost:"
    log "     rsync -avz --delete --exclude '.env' -e ssh site/ amiport-deploy:amiport.platesteel.net/"
fi
