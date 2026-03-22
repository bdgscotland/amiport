#!/bin/bash
# test-site.sh — Automated test suite for amiport website
#
# Usage: bash site/test-site.sh [URL]
# Default URL: http://localhost:8000 (starts local PHP server)
# Pass a URL to test against a live deployment: bash site/test-site.sh http://amiport.platesteel.net

set -euo pipefail

PASS=0
FAIL=0
TOTAL=0
BASE_URL="${1:-}"
LOCAL_PID=""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

cleanup() {
    if [ -n "$LOCAL_PID" ]; then
        kill "$LOCAL_PID" 2>/dev/null || true
        wait "$LOCAL_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Start local server if no URL provided
if [ -z "$BASE_URL" ]; then
    echo "Starting local PHP server..."
    cd "$(dirname "$0")"
    php -S localhost:8000 > /dev/null 2>&1 &
    LOCAL_PID=$!
    sleep 1
    BASE_URL="http://localhost:8000"
    echo "Local server at $BASE_URL (PID $LOCAL_PID)"
fi

echo ""
echo "Testing: $BASE_URL"
echo "================================"

assert() {
    local name="$1"
    local expected="$2"
    local actual="$3"
    TOTAL=$((TOTAL + 1))
    if [ "$actual" = "$expected" ]; then
        echo -e "  ${GREEN}PASS${NC} $name"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}FAIL${NC} $name (expected=$expected, got=$actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_contains() {
    local name="$1"
    local needle="$2"
    local haystack="$3"
    TOTAL=$((TOTAL + 1))
    if echo "$haystack" | grep -q "$needle"; then
        echo -e "  ${GREEN}PASS${NC} $name"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}FAIL${NC} $name (missing: $needle)"
        FAIL=$((FAIL + 1))
    fi
}

# --- Static Pages ---
echo ""
echo "Static Pages"
echo "------------"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/")
assert "Landing page returns 200" "200" "$STATUS"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/packages.html")
assert "Packages page returns 200" "200" "$STATUS"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/stats.html")
assert "Stats page returns 200" "200" "$STATUS"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/admin.php")
assert "Admin page returns 200" "200" "$STATUS"

BODY=$(curl -s "$BASE_URL/")
assert_contains "Landing page has h1" "sr-only" "$BODY"
assert_contains "Landing page has AmigaShell" "AmigaShell" "$BODY"
assert_contains "Landing page has request form" "request-form" "$BODY"
assert_contains "Landing page has stats nav link" "stats.html" "$BODY"

# --- API Health ---
echo ""
echo "API Health"
echo "----------"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/index.php")
assert "API health returns 200" "200" "$STATUS"

BODY=$(curl -s "$BASE_URL/api/v1/index.php")
assert_contains "API health has status ok" '"ok"' "$BODY"

# --- Packages API ---
echo ""
echo "Packages API"
echo "------------"

BODY=$(curl -s "$BASE_URL/api/v1/packages.php")
assert_contains "Packages list has version field" '"version"' "$BODY"
assert_contains "Packages list has packages array" '"packages"' "$BODY"

# Check download/vote fields are present
assert_contains "Packages have downloads field" '"downloads"' "$BODY"
assert_contains "Packages have vote_score field" '"vote_score"' "$BODY"

# Single package lookup
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/packages.php?name=grep")
assert "Single package returns 200" "200" "$STATUS"

BODY=$(curl -s "$BASE_URL/api/v1/packages.php?name=grep")
assert_contains "Single package has name" '"name"' "$BODY"

# Package not found
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/packages.php?name=nonexistent")
assert "Missing package returns 404" "404" "$STATUS"

# Path traversal attempt (Apache may return 500, PHP returns 404 — both are secure)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/packages.php?name=../../../etc/passwd")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "404" ] || [ "$STATUS" = "500" ] || [ "$STATUS" = "400" ]; then
    echo -e "  ${GREEN}PASS${NC} Path traversal blocked ($STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}FAIL${NC} Path traversal not blocked (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# --- Vote API ---
echo ""
echo "Vote API"
echo "--------"

# Valid vote
BODY=$(curl -s -X POST "$BASE_URL/api/v1/vote.php" \
    -H "Content-Type: application/json" \
    -d '{"package":"grep","vote":1}')
assert_contains "Vote up succeeds" '"ok":true' "$BODY"
assert_contains "Vote returns score" '"score"' "$BODY"

# Invalid vote value
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE_URL/api/v1/vote.php" \
    -H "Content-Type: application/json" \
    -d '{"package":"grep","vote":99}')
assert "Invalid vote value returns 400" "400" "$STATUS"

# Missing package name
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE_URL/api/v1/vote.php" \
    -H "Content-Type: application/json" \
    -d '{"package":"","vote":1}')
assert "Empty package name returns 400" "400" "$STATUS"

# Invalid JSON
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE_URL/api/v1/vote.php" \
    -H "Content-Type: application/json" \
    -d 'not json')
assert "Invalid JSON returns 400" "400" "$STATUS"

# Wrong method
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/vote.php")
assert "GET on vote returns 405" "405" "$STATUS"

# --- Port Request API ---
echo ""
echo "Port Request API"
echo "----------------"

# Valid request
BODY=$(curl -s -X POST "$BASE_URL/api/v1/request.php" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"testutil","tool_url":"https://example.com","website":""}')
assert_contains "Port request succeeds" '"ok":true' "$BODY"

# Honeypot triggered (bot)
BODY=$(curl -s -X POST "$BASE_URL/api/v1/request.php" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"spam","website":"http://spam.example.com"}')
assert_contains "Honeypot returns fake success" '"ok":true' "$BODY"

# Empty tool name
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE_URL/api/v1/request.php" \
    -H "Content-Type: application/json" \
    -d '{"tool_name":"","website":""}')
assert "Empty tool name returns 400" "400" "$STATUS"

# --- Stats API ---
echo ""
echo "Stats API"
echo "---------"

BODY=$(curl -s "$BASE_URL/api/v1/stats.php")
assert_contains "Stats has total_downloads" '"total_downloads"' "$BODY"
assert_contains "Stats has total_packages" '"total_packages"' "$BODY"
assert_contains "Stats has trends" '"trends"' "$BODY"
assert_contains "Stats has popular" '"popular"' "$BODY"

# --- Download API ---
echo ""
echo "Download API"
echo "------------"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/download.php?name=grep")
assert "Download grep returns 200" "200" "$STATUS"

# Missing name
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/download.php")
assert "Download without name returns 400" "400" "$STATUS"

# Invalid name (Apache may return 500, PHP returns 404 — both are secure)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/api/v1/download.php?name=../../etc/passwd")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "404" ] || [ "$STATUS" = "500" ] || [ "$STATUS" = "400" ]; then
    echo -e "  ${GREEN}PASS${NC} Download path traversal blocked ($STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}FAIL${NC} Download path traversal not blocked (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# --- Security Headers ---
echo ""
echo "Security Headers"
echo "----------------"

HEADERS=$(curl -sI "$BASE_URL/")
assert_contains "X-Content-Type-Options header" "nosniff" "$HEADERS"
assert_contains "X-Frame-Options header" "DENY" "$HEADERS"

# --- Admin Security ---
echo ""
echo "Admin Security"
echo "--------------"

# Wrong password (may hit rate limit from repeated test runs)
BODY=$(curl -s -X POST "$BASE_URL/admin.php" -d "password=wrongpassword")
TOTAL=$((TOTAL + 1))
if echo "$BODY" | grep -q "Invalid password\|Too many login attempts"; then
    echo -e "  ${GREEN}PASS${NC} Wrong password shows error"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}FAIL${NC} Wrong password shows error (no error message found)"
    FAIL=$((FAIL + 1))
fi

# --- Summary ---
echo ""
echo "================================"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}, $TOTAL total"

if [ "$FAIL" -gt 0 ]; then
    echo -e "${RED}SOME TESTS FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}ALL TESTS PASSED${NC}"
    exit 0
fi
