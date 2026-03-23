#!/usr/bin/env bash
set -euo pipefail

# check-port-metadata.sh — CI gate: validate port metadata consistency
#
# For each port in ports/*/:
#   1. Required files exist (Makefile, PORT.md, .readme, test-fsemu-cases.txt, source files)
#   2. No template placeholders remain (__PLACEHOLDER__ patterns)
#   2b. Short description <= 40 chars (Aminet requirement)
#   2c. No hallucinated Replaces: lines in .readme
#   2d. Short description is ASCII-only
#   3. Version consistency across Makefile, .readme, PORT.md, $VER string
#   4. PORTS.md entry exists
#   5. TEST-REPORT.md quality (if present)
#   6. No stray build artifacts (.lha, gmon.out, *_native, *.map, *.o in ported/)
#   7. README↔PORTS.md Aminet status consistency
#
# Exit 0 if all ports pass, exit 1 if any fail.

PORTS_DIR="${PORTS_DIR:-ports}"
PORTS_CATALOG="PORTS.md"
failed=0
warned=0
checked=0

# Verify PORTS.md exists
if [ ! -f "$PORTS_CATALOG" ]; then
    echo "FATAL: $PORTS_CATALOG not found at project root"
    exit 1
fi

for dir in "$PORTS_DIR"/*/; do
    name=$(basename "$dir")

    # Skip non-port directories
    if [ ! -f "$dir/Makefile" ]; then
        continue
    fi
    if [ "$name" = "templates" ] || [ "$name" = "common-test-data" ]; then
        continue
    fi

    checked=$((checked + 1))
    port_failed=0
    port_warned=0

    # ----------------------------------------------------------
    # Check 1: Required files
    # ----------------------------------------------------------
    missing=""
    [ ! -f "$dir/Makefile" ] && missing="$missing Makefile"
    [ ! -f "$dir/PORT.md" ] && missing="$missing PORT.md"
    [ ! -f "$dir/${name}.readme" ] && missing="$missing ${name}.readme"
    [ ! -f "$dir/test-fsemu-cases.txt" ] && missing="$missing test-fsemu-cases.txt"

    has_original=$(find "$dir/original" -name '*.c' 2>/dev/null | head -1)
    [ -z "$has_original" ] && missing="$missing original/*.c"

    has_ported=$(find "$dir/ported" -name '*.c' 2>/dev/null | head -1)
    [ -z "$has_ported" ] && missing="$missing ported/*.c"

    if [ -n "$missing" ]; then
        echo "FAIL  $name: required files — missing:$missing"
        port_failed=1
    else
        echo "PASS  $name: required files"
    fi

    # ----------------------------------------------------------
    # Check 2: No template placeholders
    # ----------------------------------------------------------
    # Known template placeholders (from ports/templates/*.template)
    placeholder_re='__TARGET__|__VERSION__|__SOURCE_URL__|__SOURCE_VERSION__|__CATEGORY__|__CATEGORY_NAME__|__LICENSE__|__AUTHOR__|__DATE_ISO__|__DESCRIPTION__|__AMINET_CAT__'
    placeholders=""
    for f in "$dir/PORT.md" "$dir/Makefile" "$dir/${name}.readme"; do
        if [ -f "$f" ]; then
            found=$(grep -oE "$placeholder_re" "$f" 2>/dev/null | sort -u | tr '\n' ' ' || true)
            [ -n "$found" ] && placeholders="$placeholders $(basename "$f"):$found"
        fi
    done

    if [ -n "$placeholders" ]; then
        echo "FAIL  $name: placeholders remain —$placeholders"
        port_failed=1
    else
        echo "PASS  $name: no placeholders"
    fi

    # ----------------------------------------------------------
    # Check 2b: Short description length (Aminet max: 40 chars)
    # ----------------------------------------------------------
    desc=$(grep -E '^DESCRIPTION\s*=' "$dir/Makefile" 2>/dev/null | head -1 | sed 's/^DESCRIPTION[[:space:]]*=[[:space:]]*//' || true)
    if [ -n "$desc" ]; then
        desc_len=${#desc}
        if [ "$desc_len" -gt 40 ]; then
            echo "FAIL  $name: description — $desc_len chars (max 40): \"$desc\""
            port_failed=1
        else
            echo "PASS  $name: description ($desc_len chars)"
        fi
    else
        echo "WARN  $name: description — no DESCRIPTION in Makefile"
        port_warned=1
    fi

    # ----------------------------------------------------------
    # Check 2c: No hallucinated Replaces: in .readme
    # ----------------------------------------------------------
    # Replaces: must ONLY be used when upgrading an EXISTING Aminet package.
    # First-time uploads must NOT have a Replaces: line.
    replaces_line=$(grep -E '^Replaces:' "$dir/${name}.readme" 2>/dev/null || true)
    if [ -n "$replaces_line" ]; then
        echo "WARN  $name: .readme has Replaces: line — verify this package exists on Aminet"
        port_warned=1
    fi

    # ----------------------------------------------------------
    # Check 2c2: Description consistency (Makefile vs .readme Short:)
    # ----------------------------------------------------------
    if [ -n "$desc" ] && [ -f "$dir/${name}.readme" ]; then
        readme_short=$(grep -E '^Short:' "$dir/${name}.readme" 2>/dev/null | head -1 | sed 's/^Short:[[:space:]]*//' || true)
        if [ -n "$readme_short" ] && [ "$readme_short" != "$desc" ]; then
            echo "WARN  $name: description mismatch — Makefile=\"$desc\" vs .readme=\"$readme_short\""
            port_warned=1
        fi
    fi

    # ----------------------------------------------------------
    # Check 2d: Short description is ASCII-only (Aminet requirement)
    # ----------------------------------------------------------
    if [ -n "$desc" ]; then
        non_ascii=$(echo "$desc" | LC_ALL=C grep -P '[^\x00-\x7F]' 2>/dev/null || true)
        if [ -n "$non_ascii" ]; then
            echo "FAIL  $name: description — contains non-ASCII characters (Aminet requires ASCII)"
            port_failed=1
        fi
    fi

    # ----------------------------------------------------------
    # Check 3: Version consistency
    # ----------------------------------------------------------
    # Extract from Makefile (VERSION = X.Y or VERSION=X.Y)
    ver_makefile=$(grep -E '^VERSION\s*=' "$dir/Makefile" 2>/dev/null | head -1 | sed 's/.*=\s*//' | tr -d ' ' || true)

    # Extract from .readme (Version:      X.Y)
    ver_readme=""
    if [ -f "$dir/${name}.readme" ]; then
        ver_readme=$(grep -E '^Version:' "$dir/${name}.readme" 2>/dev/null | head -1 | sed 's/Version:\s*//' | tr -d ' ' || true)
    fi

    # Extract from PORT.md (| Version | X.Y |)
    ver_portmd=""
    if [ -f "$dir/PORT.md" ]; then
        ver_portmd=$(grep -E '^\| Version \|' "$dir/PORT.md" 2>/dev/null | head -1 | awk -F'|' '{print $3}' | tr -d ' ' || true)
    fi

    # Extract from $VER string in ported/*.c
    ver_source=""
    ver_count=0
    ver_versions=""
    ver_conflict=false
    if [ -d "$dir/ported" ]; then
        while IFS= read -r line; do
            ver_count=$((ver_count + 1))
            # Extract version: $VER: name X.Y (date) -> X.Y
            v=$(echo "$line" | sed -E 's/.*\$VER: [^ ]+ ([^ ]+) .*/\1/')
            ver_versions="$ver_versions $v"
            [ -z "$ver_source" ] && ver_source="$v"
        done < <(grep -rh '\$VER:' "$dir/ported/"*.c 2>/dev/null || true)
    fi

    # Check $VER count and consistency
    if [ "$ver_count" -eq 0 ]; then
        echo "FAIL  $name: version — no \$VER string in ported/*.c"
        port_failed=1
    else
        # Check all $VER strings agree
        ver_conflict=false
        for v in $ver_versions; do
            if [ "$v" != "$ver_source" ]; then
                ver_conflict=true
            fi
        done
        if [ "$ver_conflict" = true ]; then
            echo "FAIL  $name: version — conflicting \$VER strings:$ver_versions"
            port_failed=1
        fi
    fi

    # Compare available versions (skip sources where version not found)
    if [ -n "$ver_makefile" ] && [ "$ver_count" -gt 0 ] && [ "$ver_conflict" = false ]; then
        mismatch=""
        [ -n "$ver_readme" ] && [ "$ver_readme" != "$ver_makefile" ] && mismatch="$mismatch .readme=$ver_readme"
        [ -n "$ver_portmd" ] && [ "$ver_portmd" != "$ver_makefile" ] && mismatch="$mismatch PORT.md=$ver_portmd"
        [ -n "$ver_source" ] && [ "$ver_source" != "$ver_makefile" ] && mismatch="$mismatch \$VER=$ver_source"

        if [ -n "$mismatch" ]; then
            echo "FAIL  $name: version — Makefile=$ver_makefile but$mismatch"
            port_failed=1
        elif [ -z "$ver_portmd" ]; then
            echo "WARN  $name: version — consistent ($ver_makefile) but PORT.md has no version row"
            port_warned=1
        else
            echo "PASS  $name: version ($ver_makefile)"
        fi
    elif [ -z "$ver_makefile" ]; then
        echo "FAIL  $name: version — no VERSION in Makefile"
        port_failed=1
    fi

    # ----------------------------------------------------------
    # Check 3b: Revision consistency (optional field)
    # ----------------------------------------------------------
    rev_makefile=$(grep -E '^REVISION\s*=' "$dir/Makefile" 2>/dev/null | head -1 | sed 's/.*=\s*//' | tr -d ' ' || true)
    rev_makefile="${rev_makefile:-1}"
    if [ "$rev_makefile" -gt 0 ] 2>/dev/null; then
        echo "PASS  $name: revision ($rev_makefile)"
    else
        echo "WARN  $name: revision — invalid REVISION value in Makefile"
        port_warned=1
    fi

    # ----------------------------------------------------------
    # Check 4: PORTS.md entry
    # ----------------------------------------------------------
    if grep -qE "^\|[[:space:]]*\[?${name}\]?" "$PORTS_CATALOG" 2>/dev/null; then
        echo "PASS  $name: PORTS.md entry"
    else
        echo "FAIL  $name: PORTS.md entry — not found"
        port_failed=1
    fi

    # ----------------------------------------------------------
    # Check 5: TEST-REPORT.md quality
    # ----------------------------------------------------------
    if [ -f "$dir/TEST-REPORT.md" ]; then
        has_zero=$(grep -c '0/0 passed' "$dir/TEST-REPORT.md" 2>/dev/null || true)
        # Check breakdown table has data rows (lines starting with | N | where N is a digit)
        has_data_rows=$(grep -cE '^\| [0-9]' "$dir/TEST-REPORT.md" 2>/dev/null || true)

        if [ "$has_zero" -gt 0 ]; then
            echo "WARN  $name: test report — shows 0/0 passed (stale?)"
            port_warned=1
        elif [ "$has_data_rows" -eq 0 ]; then
            echo "WARN  $name: test report — empty breakdown table"
            port_warned=1
        else
            echo "PASS  $name: test report"
        fi
    else
        echo "SKIP  $name: test report (no TEST-REPORT.md)"
    fi

    # ----------------------------------------------------------
    # Check 6: Stray artifacts
    # LHA packages and versioned readmes are expected after `make package`
    # ----------------------------------------------------------
    strays=""
    for f in "$dir"/gmon.out; do [ -f "$f" ] && strays="$strays gmon.out"; done
    for f in "$dir"/*_native; do [ -f "$f" ] && strays="$strays $(basename "$f")"; done
    for f in "$dir"/*.map; do [ -f "$f" ] && strays="$strays $(basename "$f")"; done
    # Check for .o files in ported/ (build artifacts)
    for f in "$dir"/ported/*.o; do [ -f "$f" ] && strays="$strays ported/$(basename "$f")"; done

    if [ -n "$strays" ]; then
        echo "FAIL  $name: stray artifacts —$strays"
        port_failed=1
    else
        echo "PASS  $name: no stray artifacts"
    fi

    # ----------------------------------------------------------
    # Check 7: README↔PORTS.md Aminet status consistency
    # ----------------------------------------------------------
    # Extract Aminet column from PORTS.md catalog table (preserve internal spaces)
    ports_aminet=$(grep -E "^\|[[:space:]]*\[?${name}\]?" "$PORTS_CATALOG" | head -1 | awk -F'|' '{print $8}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || true)

    # Extract status column from README.md ports table
    readme_status=""
    if [ -f "README.md" ]; then
        readme_status=$(grep -E "^\|[[:space:]]*\[${name}\]" "README.md" | head -1 | awk -F'|' '{print $6}' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' || true)
    fi

    if [ -n "$ports_aminet" ] && [ -n "$readme_status" ]; then
        # Check: if PORTS.md says "Submitted <date>" (not "Not submitted") but README doesn't mention Aminet
        ports_submitted=false
        readme_submitted=false
        # Match "Submitted 2026-..." or aminet.net URL, but NOT "Not submitted"
        echo "$ports_aminet" | grep -qiE "^Submitted[[:space:]]+[0-9]|aminet.net" && ports_submitted=true
        echo "$readme_status" | grep -qi "aminet" && readme_submitted=true

        if [ "$ports_submitted" = true ] && [ "$readme_submitted" = false ]; then
            echo "FAIL  $name: Aminet status — PORTS.md says submitted but README.md says '$readme_status'"
            port_failed=1
        elif [ "$ports_submitted" = false ] && [ "$readme_submitted" = true ]; then
            echo "FAIL  $name: Aminet status — README.md says Aminet but PORTS.md says '$ports_aminet'"
            port_failed=1
        else
            echo "PASS  $name: Aminet status consistent"
        fi
    fi

    # Separator between ports
    [ "$port_failed" -gt 0 ] && failed=$((failed + 1))
    [ "$port_warned" -gt 0 ] && [ "$port_failed" -eq 0 ] && warned=$((warned + 1))
    echo ""
done

echo "Checked $checked ports: $((checked - failed - warned)) clean, $warned warnings, $failed failed"

if [ "$failed" -gt 0 ]; then
    exit 1
fi
