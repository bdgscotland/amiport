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
#   8. Catalog orphans: ports/ dirs that aren't in ported[] of catalog.json
#   8b. Catalog duplicates: same name in ported[] twice, or in both candidates[] and ported[]
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

    # Build the display version: VERSION for rev 1, VERSION-REVISION for rev 2+
    # This is what $VER strings and .readme should contain
    rev_for_display=$(grep -E '^REVISION\s*=' "$dir/Makefile" 2>/dev/null | head -1 | sed 's/.*=\s*//' | tr -d ' ' || true)
    rev_for_display="${rev_for_display:-1}"
    if [ "$rev_for_display" != "1" ] && [ "$rev_for_display" -gt 1 ] 2>/dev/null; then
        display_version="${ver_makefile}-${rev_for_display}"
    else
        display_version="$ver_makefile"
    fi

    # Compare available versions (skip sources where version not found)
    # $VER and .readme should match display_version (includes revision when > 1)
    # PORT.md can match either ver_makefile or display_version
    if [ -n "$ver_makefile" ] && [ "$ver_count" -gt 0 ] && [ "$ver_conflict" = false ]; then
        mismatch=""
        if [ -n "$ver_readme" ]; then
            [ "$ver_readme" != "$display_version" ] && [ "$ver_readme" != "$ver_makefile" ] && mismatch="$mismatch .readme=$ver_readme"
        fi
        if [ -n "$ver_portmd" ]; then
            [ "$ver_portmd" != "$display_version" ] && [ "$ver_portmd" != "$ver_makefile" ] && mismatch="$mismatch PORT.md=$ver_portmd"
        fi
        if [ -n "$ver_source" ]; then
            [ "$ver_source" != "$display_version" ] && [ "$ver_source" != "$ver_makefile" ] && mismatch="$mismatch \$VER=$ver_source"
        fi

        if [ -n "$mismatch" ]; then
            echo "FAIL  $name: version — expected=$display_version but$mismatch"
            port_failed=1
        elif [ -z "$ver_portmd" ]; then
            echo "WARN  $name: version — consistent ($display_version) but PORT.md has no version row"
            port_warned=1
        else
            echo "PASS  $name: version ($display_version)"
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
    # Check 4b: README.md ports table entry
    # ----------------------------------------------------------
    if [ -f "README.md" ]; then
        if grep -qE "^\|[[:space:]]*\[${name}\]" "README.md" 2>/dev/null; then
            echo "PASS  $name: README.md ports table entry"
        else
            echo "FAIL  $name: README.md ports table entry — not found"
            port_failed=1
        fi
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

# ----------------------------------------------------------
# Check 8: Catalog orphans — ports/ dirs missing from ported[] in catalog.json
# ----------------------------------------------------------
CATALOG_FILE="${CATALOG_FILE:-data/catalog.json}"
if [ -f "$CATALOG_FILE" ]; then
    echo "--- Catalog orphan check ---"
    catalog_orphans=0
    for dir in "$PORTS_DIR"/*/; do
        name=$(basename "$dir")
        [ ! -f "$dir/Makefile" ] && continue
        [ "$name" = "templates" ] || [ "$name" = "common-test-data" ] && continue

        # Check if name appears in ported[] array
        in_ported=$(python3 -c "
import json, sys
with open('$CATALOG_FILE') as f:
    cat = json.load(f)
for p in cat.get('ported', []):
    if p['name'] == '$name':
        sys.exit(0)
sys.exit(1)
" 2>/dev/null && echo "yes" || echo "no")

        if [ "$in_ported" = "no" ]; then
            # Also check if it's in candidates (stale entry)
            in_candidates=$(python3 -c "
import json, sys
with open('$CATALOG_FILE') as f:
    cat = json.load(f)
for c in cat.get('candidates', []):
    if c['name'] == '$name':
        sys.exit(0)
sys.exit(1)
" 2>/dev/null && echo "yes" || echo "no")

            # If there's a compiled binary AND test suite, this is a completed port — hard fail
            # Otherwise it's WIP — just warn
            if [ -f "$dir/$name" ] && [ -f "$dir/test-fsemu-cases.txt" ]; then
                if [ "$in_candidates" = "yes" ]; then
                    echo "FAIL  $name: catalog — completed port still in candidates[] (should be in ported[])"
                    failed=$((failed + 1))
                else
                    echo "WARN  $name: catalog — completed port missing from catalog.json entirely"
                    warned=$((warned + 1))
                fi
            else
                echo "WARN  $name: catalog — port directory exists without binary (WIP?) and no catalog entry"
                warned=$((warned + 1))
            fi
            catalog_orphans=$((catalog_orphans + 1))
        fi
    done
    if [ "$catalog_orphans" -eq 0 ]; then
        echo "PASS  all ports have catalog.json ported[] entries"
    fi
    echo ""

    # ----------------------------------------------------------
    # Check 8b: Catalog duplicates — same name in ported[] more than once,
    # or same name in BOTH candidates[] and ported[]
    # ----------------------------------------------------------
    echo "--- Catalog duplicate check ---"
    catalog_dupes=$(CATALOG_FILE="$CATALOG_FILE" python3 -c '
import json, sys, os
cf = os.environ.get("CATALOG_FILE", "data/catalog.json")
with open(cf) as f:
    cat = json.load(f)
dupes = 0
ported_names = {}
for p in cat.get("ported", []):
    n = p["name"]
    ported_names[n] = ported_names.get(n, 0) + 1
for n, count in sorted(ported_names.items()):
    if count > 1:
        print("FAIL  %s: appears %d times in ported[]" % (n, count))
        dupes += 1
cand_names = set(c["name"] for c in cat.get("candidates", []))
for n in sorted(cand_names & set(ported_names)):
    print("FAIL  %s: in BOTH candidates[] and ported[]" % n)
    dupes += 1
if dupes == 0:
    print("PASS  no duplicate catalog entries")
' || true)
    echo "$catalog_dupes"
    if echo "$catalog_dupes" | grep -q '^FAIL'; then
        failed=$((failed + 1))
    fi
    echo ""
fi

# ----------------------------------------------------------
# Check 9: Site mirror integrity — every advertised LHA must exist
#          in site/packages/ before deploy can rsync --delete safely
# ----------------------------------------------------------
SITE_DATA_DIR="${SITE_DATA_DIR:-site/data/packages}"
SITE_PKGS_DIR="${SITE_PKGS_DIR:-site/packages}"
if [ -d "$SITE_DATA_DIR" ] && [ -d "$SITE_PKGS_DIR" ]; then
    echo "--- Site mirror integrity check ---"
    mirror_report=$(python3 - "$SITE_DATA_DIR" "$SITE_PKGS_DIR" "$PORTS_DIR" <<'PY' || true
import json, os, sys
data_dir, pkgs_dir, ports_dir = sys.argv[1], sys.argv[2], sys.argv[3]
fail = 0
warn = 0
for entry in sorted(os.listdir(data_dir)):
    if not entry.endswith('.json'):
        continue
    path = os.path.join(data_dir, entry)
    try:
        d = json.load(open(path))
    except Exception as e:
        print(f"WARN  {entry}: invalid JSON ({e})")
        warn += 1
        continue
    name = d.get('name') or os.path.splitext(entry)[0]
    download = d.get('download', '')
    if not download or not download.startswith('/packages/'):
        # No advertised LHA path -- skip silently (e.g. installer placeholder)
        continue
    lha = download[len('/packages/'):]
    expected = os.path.join(pkgs_dir, lha)
    if os.path.exists(expected):
        continue
    # Missing from site mirror -- can we recover from the port directory?
    port_copy = os.path.join(ports_dir, name, lha)
    if os.path.exists(port_copy):
        print(f"FAIL  {name}: site mirror missing {lha}")
        print(f"      fix: cp {port_copy} {expected}")
    else:
        print(f"FAIL  {name}: site mirror missing {lha} AND no copy in {ports_dir}/{name}/")
        print(f"      this artifact may have been lost -- rebuild or roll back JSON")
    fail += 1
if fail == 0:
    print(f"PASS  site mirror integrity (every advertised LHA exists in {pkgs_dir}/)")
sys.exit(1 if fail > 0 else 0)
PY
)
    echo "$mirror_report"
    if echo "$mirror_report" | grep -q '^FAIL'; then
        failed=$((failed + 1))
    fi
    echo ""
fi

# ----------------------------------------------------------
# Check 10: Catalog ↔ Makefile revision consistency
#           Catches phantom revision drift where site/data/packages/<name>.json
#           advertises a revision that the port source never actually shipped.
#           Real incident (2026-04-14): lua.json had revision: 2 but
#           ports/lua/Makefile said rev 1, no rev-2 work existed anywhere,
#           and no rev-2 LHA was ever built.
# ----------------------------------------------------------
if [ -d "$SITE_DATA_DIR" ] && [ -d "$PORTS_DIR" ]; then
    echo "--- Catalog revision consistency check ---"
    rev_report=$(python3 - "$SITE_DATA_DIR" "$PORTS_DIR" <<'PY' || true
import json, os, re, sys
data_dir, ports_dir = sys.argv[1], sys.argv[2]
fail = 0
for entry in sorted(os.listdir(data_dir)):
    if not entry.endswith('.json'):
        continue
    path = os.path.join(data_dir, entry)
    try:
        d = json.load(open(path))
    except Exception:
        continue
    name = d.get('name') or os.path.splitext(entry)[0]
    json_rev = int(d.get('revision', 1))
    json_ver = d.get('version', '')

    mf = os.path.join(ports_dir, name, 'Makefile')
    if not os.path.exists(mf):
        # Native artifact with no port directory -- skip silently
        continue
    mf_text = open(mf).read()

    mf_ver_match = re.search(r'^VERSION\s*=\s*(\S+)', mf_text, re.M)
    mf_rev_match = re.search(r'^REVISION\s*=\s*(\d+)', mf_text, re.M)
    mf_ver = mf_ver_match.group(1) if mf_ver_match else ''
    # No REVISION line means rev 1 (project convention)
    mf_rev = int(mf_rev_match.group(1)) if mf_rev_match else 1

    if mf_ver and json_ver and mf_ver != json_ver:
        print(f"FAIL  {name}: catalog version drift -- json={json_ver} Makefile={mf_ver}")
        fail += 1
        continue
    if json_rev != mf_rev:
        print(f"FAIL  {name}: catalog revision drift -- json={json_rev} Makefile={mf_rev}")
        print(f"      fix one of:")
        print(f"        - {data_dir}/{name}.json: set \"revision\": {mf_rev}")
        print(f"        - {ports_dir}/{name}/Makefile: set REVISION = {json_rev}")
        print(f"        (whichever reflects the actual work that was done)")
        fail += 1
if fail == 0:
    print(f"PASS  catalog revision consistency (json revision == Makefile REVISION for all ports)")
sys.exit(1 if fail > 0 else 0)
PY
)
    echo "$rev_report"
    if echo "$rev_report" | grep -q '^FAIL'; then
        failed=$((failed + 1))
    fi
    echo ""
fi

echo "Checked $checked ports: $((checked - failed - warned)) clean, $warned warnings, $failed failed"

if [ "$failed" -gt 0 ]; then
    exit 1
fi
