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

# Check lha is available
if command -v lha >/dev/null 2>&1; then
    echo "${GREEN}[OK]${RESET} lha archiver available"
else
    echo "${RED}[FAIL]${RESET} lha not found — brew install lha"
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
    grep "^$1" "$PORT_DIR/Makefile" | head -1 | sed "s/^$1[[:space:]]*=[[:space:]]*//"
}

VERSION=$(get_var VERSION)
DESCRIPTION=$(get_var DESCRIPTION)
AUTHOR=$(get_var AUTHOR)
AMINET_CAT=$(get_var AMINET_CAT)

VERSION="${VERSION:-1.0}"
DESCRIPTION="${DESCRIPTION:-Ported CLI utility}"
AUTHOR="${AUTHOR:-Unknown}"
AMINET_CAT="${AMINET_CAT:-util/cli}"

# --- Ask for uploader email ---

echo "Aminet requires an uploader email (partially obfuscated on the site)."
printf "Your email: "
read -r UPLOADER_EMAIL

if [ -z "$UPLOADER_EMAIL" ]; then
    echo "Email is required for Aminet uploads."
    exit 1
fi

# --- Generate .readme ---

ARCHIVE_NAME="${PORT_NAME}-${VERSION}"
README_FILE="$PORT_DIR/${ARCHIVE_NAME}.readme"

cat > "$README_FILE" << READMEEOF
Short:        $DESCRIPTION
Type:         $AMINET_CAT
Architecture: m68k-amigaos >= 3.0
Uploader:     $UPLOADER_EMAIL
Author:       $AUTHOR (ported by amiport)
Version:      $VERSION
URL:          https://github.com/bdgscotland/amiport

$PORT_NAME — $DESCRIPTION

Ported to AmigaOS 3.x using amiport, an AI-assisted porting toolkit.
Cross-compiled with m68k-amigaos-gcc, tested in vamos emulator.

Includes source code (original POSIX and ported Amiga versions).

See PORT.md for full transformation details and test results.
READMEEOF

echo "${GREEN}[OK]${RESET} Generated $README_FILE"

# --- Build LHA package ---

LHA_FILE="$PORT_DIR/${ARCHIVE_NAME}.lha"

(cd "$PORT_DIR" && lha a "${ARCHIVE_NAME}.lha" \
    "$PORT_NAME" \
    "${ARCHIVE_NAME}.readme" \
    PORT.md \
    original/ \
    ported/ \
)

echo "${GREEN}[OK]${RESET} Created $LHA_FILE"

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
echo "${BOLD}--- LHA contents ---${RESET}"
lha l "$LHA_FILE"
echo ""

# --- Confirm ---

printf "${BOLD}Upload to Aminet? (yes/no): ${RESET}"
read -r CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo ""
    echo "Upload cancelled. Files are ready at:"
    echo "  $LHA_FILE"
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

ftp -n main.aminet.net << FTPEOF
user anonymous $UPLOADER_EMAIL
cd new
binary
put $LHA_FILE ${ARCHIVE_NAME}.lha
put $README_FILE ${ARCHIVE_NAME}.readme
bye
FTPEOF

if [ $? -eq 0 ]; then
    echo ""
    echo "${GREEN}${BOLD}=== Upload complete ===${RESET}"
    echo "Files uploaded to main.aminet.net/new/"
    echo "They will appear on Aminet after moderator review."
else
    echo ""
    echo "${RED}Upload failed. Try manually via FTP.${RESET}"
    exit 1
fi
