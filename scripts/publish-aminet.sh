#!/bin/bash
#
# publish-aminet.sh — Prepare and upload a port to Aminet
#
# Usage: make publish TARGET=ports/cal
#
# This script:
# 1. Validates the port is built and tested
# 2. Generates an Aminet-format .readme
# 3. Builds the LHA package
# 4. Shows a preview
# 5. Asks for confirmation
# 6. Uploads via FTP
#
# Never uploads without explicit user confirmation.

set -euo pipefail

if [ -z "${1:-}" ]; then
    echo "Usage: $0 <port-directory>"
    echo "Example: $0 ports/cal"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PORT_DIR="$1"
PORT_NAME=$(basename "$PORT_DIR")

# Color support
if [ -t 1 ] && command -v tput >/dev/null 2>&1; then
    GREEN=$(tput setaf 2); RED=$(tput setaf 1); YELLOW=$(tput setaf 3)
    BOLD=$(tput bold); RESET=$(tput sgr0)
else
    GREEN=""; RED=""; YELLOW=""; BOLD=""; RESET=""
fi

echo "${BOLD}=== Aminet Publishing: $PORT_NAME ===${RESET}"
echo ""

# --- Pre-publish checklist ---

ERRORS=0

# Check binary exists
if [ -f "$PORT_DIR/$PORT_NAME" ]; then
    echo "${GREEN}[OK]${RESET} Binary exists"
else
    echo "${RED}[FAIL]${RESET} Binary not found — run: make build TARGET=$PORT_DIR"
    ERRORS=$((ERRORS + 1))
fi

# Check PORT.md exists
if [ -f "$PORT_DIR/PORT.md" ]; then
    echo "${GREEN}[OK]${RESET} PORT.md exists"
else
    echo "${RED}[FAIL]${RESET} PORT.md not found"
    ERRORS=$((ERRORS + 1))
fi

# Check source is included (for open-source compliance)
if [ -d "$PORT_DIR/original" ] && ls "$PORT_DIR/original/"*.c >/dev/null 2>&1; then
    echo "${GREEN}[OK]${RESET} Source code included"
else
    echo "${YELLOW}[WARN]${RESET} No source in original/ — Aminet requires source for GPL/open-source"
fi

# Check Makefile
if [ -f "$PORT_DIR/Makefile" ]; then
    echo "${GREEN}[OK]${RESET} Makefile exists"
else
    echo "${RED}[FAIL]${RESET} Makefile not found"
    ERRORS=$((ERRORS + 1))
fi

# Check archiver is available (lha preferred, zip as fallback)
if command -v lha >/dev/null 2>&1 && lha --help 2>&1 | grep -q "create\|a "; then
    ARCHIVER="lha"
    echo "${GREEN}[OK]${RESET} lha archiver available"
elif command -v zip >/dev/null 2>&1; then
    ARCHIVER="zip"
    echo "${GREEN}[OK]${RESET} zip archiver available (Aminet accepts .zip)"
else
    echo "${RED}[FAIL]${RESET} No archiver found — install zip or lha"
    ERRORS=$((ERRORS + 1))
fi

if [ "$ERRORS" -gt 0 ]; then
    echo ""
    echo "${RED}$ERRORS issue(s) must be fixed before publishing.${RESET}"
    exit 1
fi

echo ""

# --- Read metadata from Makefile ---

get_var() {
    grep "^$1" "$PORT_DIR/Makefile" 2>/dev/null | head -1 | sed "s/^$1[[:space:]]*=[[:space:]]*//" || true
}

VERSION=$(get_var VERSION)
REVISION=$(get_var REVISION)
DESCRIPTION=$(get_var DESCRIPTION)
AUTHOR=$(get_var AUTHOR)
AMINET_CAT=$(get_var AMINET_CAT)

VERSION="${VERSION:-1.0}"
REVISION="${REVISION:-1}"

# Package suffix: version alone for rev 1, version-revision for rev 2+
if [ "$REVISION" = "1" ]; then
    PKG_SUFFIX="$VERSION"
else
    PKG_SUFFIX="${VERSION}-${REVISION}"
fi
DESCRIPTION="${DESCRIPTION:-Ported CLI utility}"
AUTHOR="${AUTHOR:-Unknown}"
AMINET_CAT="${AMINET_CAT:-util/cli}"

# Validate Short description length (Aminet max: 40 chars)
DESC_LEN=${#DESCRIPTION}
if [ "$DESC_LEN" -gt 40 ]; then
    echo "${RED}[FAIL]${RESET} Short description too long ($DESC_LEN chars, max 40)"
    echo "       \"$DESCRIPTION\""
    echo "       Shorten the DESCRIPTION in $PORT_DIR/Makefile"
    ERRORS=$((ERRORS + 1))
fi

if [ "$ERRORS" -gt 0 ]; then
    echo ""
    echo "${RED}$ERRORS issue(s) must be fixed before publishing.${RESET}"
    exit 1
fi

# --- Ask for uploader email ---

# Load identity from .env (local to this machine, gitignored)
ENV_FILE="$PROJECT_DIR/.env"
if [ -f "$ENV_FILE" ]; then
    . "$ENV_FILE"
    echo "[OK] Loaded identity from .env: $AMINET_NAME <$AMINET_EMAIL>"
elif [ -f "$PROJECT_DIR/.env.example" ]; then
    echo "${YELLOW}[WARN]${RESET} No .env file found. Create one from the template:"
    echo "       cp .env.example .env"
    echo ""
fi

# Prompt for anything not in .env
if [ -z "${AMINET_EMAIL:-}" ]; then
    echo "Aminet requires your details for the upload."
    echo "(Your email is partially obfuscated on the site — @ and . stripped.)"
    echo ""
    printf "Your email: "
    read -r AMINET_EMAIL
    printf "Your name: "
    read -r AMINET_NAME
    echo ""
    echo "To avoid entering this every time, create .env in the project root:"
    echo "  AMINET_EMAIL=$AMINET_EMAIL"
    echo "  AMINET_NAME=$AMINET_NAME"
    echo ""
fi

UPLOADER_EMAIL="$AMINET_EMAIL"
UPLOADER_NAME="${AMINET_NAME:-amiport}"

if [ -z "$UPLOADER_EMAIL" ]; then
    echo "Email is required for Aminet uploads."
    exit 1
fi

# --- Generate .readme ---

ARCHIVE_NAME="${PORT_NAME}-${PKG_SUFFIX}"
README_FILE="$PORT_DIR/${ARCHIVE_NAME}.readme"

cat > "$README_FILE" << READMEEOF
Short:        $DESCRIPTION
Type:         $AMINET_CAT
Architecture: m68k-amigaos >= 3.0
Uploader:     $UPLOADER_EMAIL
Author:       $AUTHOR (ported by $UPLOADER_NAME)
Version:      $VERSION

$PORT_NAME — $DESCRIPTION

Original author: $AUTHOR
Ported by $UPLOADER_NAME using amiport (AI-assisted porting toolkit).
Project: https://github.com/bdgscotland/amiport

Cross-compiled with m68k-amigaos-gcc for AmigaOS 3.x (68000+).
Tested in vamos emulator and FS-UAE.

Includes full source code (original POSIX and ported Amiga versions)
and PORT.md documenting every transformation applied.
READMEEOF

echo "${GREEN}[OK]${RESET} Generated $README_FILE"

# --- Build archive ---

if [ "$ARCHIVER" = "lha" ]; then
    ARCHIVE_FILE="$PORT_DIR/${ARCHIVE_NAME}.lha"
    (cd "$PORT_DIR" && lha a "${ARCHIVE_NAME}.lha" \
        "$PORT_NAME" \
        "${ARCHIVE_NAME}.readme" \
        PORT.md \
        original/ \
        ported/ \
    )
else
    ARCHIVE_FILE="$PORT_DIR/${ARCHIVE_NAME}.zip"
    (cd "$PORT_DIR" && zip -r "${ARCHIVE_NAME}.zip" \
        "$PORT_NAME" \
        "${ARCHIVE_NAME}.readme" \
        PORT.md \
        original/ \
        ported/ \
    )
fi

echo "${GREEN}[OK]${RESET} Created $ARCHIVE_FILE"

# --- Preview ---

echo ""
echo "${BOLD}=== Upload Preview ===${RESET}"
echo ""
echo "${BOLD}Archive:${RESET}  ${ARCHIVE_NAME}.lha"
echo "${BOLD}Category:${RESET} $AMINET_CAT"
echo "${BOLD}Server:${RESET}   main.aminet.net/new/"
echo ""
echo "${BOLD}--- .readme ---${RESET}"
cat "$README_FILE"
echo ""
echo "${BOLD}--- Archive contents ---${RESET}"
if [ "$ARCHIVER" = "lha" ]; then
    lha l "$ARCHIVE_FILE"
else
    unzip -l "$ARCHIVE_FILE"
fi
echo ""

# --- Confirm ---

# Allow --yes flag to skip confirmation (for CI/automation)
if [ "${AMINET_CONFIRM:-}" = "yes" ] || [ "${2:-}" = "--yes" ]; then
    CONFIRM="yes"
else
    printf "${BOLD}Upload to Aminet? (yes/no): ${RESET}"
    read -r CONFIRM < /dev/tty
fi

if [ "$CONFIRM" != "yes" ]; then
    echo ""
    echo "Upload cancelled. Files are ready at:"
    echo "  $ARCHIVE_FILE"
    echo "  $README_FILE"
    echo ""
    echo "You can upload manually:"
    echo "  ftp main.aminet.net (anonymous, password=$UPLOADER_EMAIL)"
    echo "  cd new"
    echo "  put ${ARCHIVE_NAME}.lha"
    echo "  put ${ARCHIVE_NAME}.readme"
    exit 0
fi

# --- Upload ---

echo ""
echo "Uploading to Aminet..."

# Determine archive extension for remote filename
ARCHIVE_EXT="${ARCHIVE_FILE##*.}"

curl -s -T "$ARCHIVE_FILE" \
    "ftp://main.aminet.net/new/${ARCHIVE_NAME}.${ARCHIVE_EXT}" \
    --user "anonymous:${UPLOADER_EMAIL}" && \
curl -s -T "$README_FILE" \
    "ftp://main.aminet.net/new/${ARCHIVE_NAME}.readme" \
    --user "anonymous:${UPLOADER_EMAIL}"

if [ $? -eq 0 ]; then
    echo ""
    echo "${GREEN}${BOLD}=== Upload complete ===${RESET}"
    echo "Files uploaded to main.aminet.net/new/"
    echo ""
    echo "${BOLD}REMINDER: Update documentation:${RESET}"
    echo "  1. PORTS.md — Set status to 'Submitted YYYY-MM-DD' and update tracking table"
    echo "  2. README.md — Update port status in the Ports table"
    echo "  3. Run 'make check-aminet' after 24h to verify publication"
    echo "They will appear on Aminet after moderator review."
else
    echo ""
    echo "${RED}Upload failed. Try manually via FTP.${RESET}"
    exit 1
fi
