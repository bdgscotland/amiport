#!/bin/bash
#
# test-fsemu.sh — Automated FS-UAE testing for amiport ports
#
# Boots FS-UAE headless, runs an ARexx test harness inside AmigaOS,
# captures TAP-format results via shared volume, reports pass/fail.
#
# Usage:
#   scripts/test-fsemu.sh ports/grep          # Test a specific port
#   scripts/test-fsemu.sh --debug ports/grep  # Test with Enforcer crash detection
#   scripts/test-fsemu.sh --all               # Test all ports with test-fsemu targets
#   make test-fsemu TARGET=ports/grep         # Via Makefile
#
# Architecture (ADR-014):
#   Host prepares a boot volume with test script + binaries
#   FS-UAE boots headless, runs ARexx test harness, writes results
#   UAEQuit shuts down emulator, host reads TAP results
#
# Prerequisites:
#   - FS-UAE installed (brew install fs-uae)
#   - Kickstart 3.1 ROM at ~/Documents/FS-UAE/Kickstarts/kick3.1.rom
#   - Bootable system at build/system/
#   - UAEQuit binary built (make build-uaequit)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SYSTEM_HDF="$PROJECT_DIR/build/system.hdf"
SYSTEM_DIR="$PROJECT_DIR/build/system"
AMIGA_DIR="$PROJECT_DIR/build/amiga"
TOOLCHAIN_DIR="$PROJECT_DIR/toolchain"
KICKSTART="$HOME/Documents/FS-UAE/Kickstarts/kick3.1.rom"
TIMEOUT_SECONDS=60
DEBUG_MODE=false
SERIAL_PID=""
ENFORCER_LOG=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

usage() {
    echo "Usage: $0 [--debug] <port-directory> [--timeout N]"
    echo "       $0 --all [--timeout N]"
    echo ""
    echo "Options:"
    echo "  --debug        Enable Enforcer crash detection (68030+MMU, serial capture)"
    echo "  --timeout N    Timeout in seconds (default: 30, or 90 in debug mode)"
    echo "  --all          Test all ports that have test-fsemu targets"
    exit 1
}

# ============================================================
# Pre-flight checks
# ============================================================
preflight() {
    local errors=0

    if ! command -v fs-uae >/dev/null 2>&1; then
        echo -e "${RED}ERROR: FS-UAE not installed. Install with: brew install fs-uae${NC}"
        errors=$((errors + 1))
    fi

    if [ ! -f "$KICKSTART" ]; then
        echo -e "${RED}ERROR: Kickstart ROM not found at $KICKSTART${NC}"
        errors=$((errors + 1))
    fi

    if [ ! -f "$SYSTEM_HDF" ] && [ ! -d "$SYSTEM_DIR/S" ]; then
        echo -e "${RED}ERROR: No bootable system — need build/system.hdf or build/system/S/${NC}"
        echo "  Run: make setup-emu"
        errors=$((errors + 1))
    fi

    if [ ! -f "$TOOLCHAIN_DIR/uaequit/UAEQuit" ]; then
        echo -e "${YELLOW}Building UAEQuit...${NC}"
        make -C "$TOOLCHAIN_DIR/uaequit" 2>&1 | tail -1
        if [ ! -f "$TOOLCHAIN_DIR/uaequit/UAEQuit" ]; then
            echo -e "${RED}ERROR: Failed to build UAEQuit${NC}"
            errors=$((errors + 1))
        fi
    fi

    # Debug mode checks
    if [ "$DEBUG_MODE" = true ]; then
        local debug_tools_dir="$TOOLCHAIN_DIR/debug-tools"
        for tool in Enforcer MungWall SegTracker; do
            if [ ! -f "$debug_tools_dir/$tool" ]; then
                echo -e "${RED}ERROR: Debug tool not found: $debug_tools_dir/$tool${NC}"
                echo "  Run: make setup-debug-tools"
                errors=$((errors + 1))
            fi
        done
    fi

    if [ $errors -gt 0 ]; then
        echo -e "${RED}Pre-flight failed with $errors error(s)${NC}"
        exit 1
    fi
}

# ============================================================
# Generate test cases from port's Makefile test-fsemu target
# ============================================================
generate_test_cases() {
    local port_dir="$1"
    local port_name
    port_name=$(basename "$port_dir")
    local cases_file="$RESULTS_DIR/test-cases.txt"

    # Check if port has a test-fsemu-cases file
    if [ -f "$port_dir/test-fsemu-cases.txt" ]; then
        cp "$port_dir/test-fsemu-cases.txt" "$cases_file"
        return 0
    fi

    # Generate basic smoke test: run the program with --help or -V
    # and verify it doesn't crash (exit code 0 or 5 or 10, not 20)
    cat > "$cases_file" << EOF
TEST: ${port_name} runs without crashing
CMD: WORK:${port_name} -V
EXPECT: grep version
EOF

    echo -e "${YELLOW}No test-fsemu-cases.txt found for $port_name — using smoke test${NC}"
}

# ============================================================
# Build the temporary boot volume
# ============================================================
build_boot_volume() {
    local port_dir="$1"
    local port_name
    port_name=$(basename "$port_dir")

    # Reuse RESULTS_DIR if already created (by serial capture setup in main)
    if [ -z "${RESULTS_DIR:-}" ] || [ ! -d "${RESULTS_DIR:-}" ]; then
        RESULTS_DIR=$(mktemp -d "${TMPDIR:-/tmp}/amiport-fsemu-XXXXXX")
    fi

    # Install binaries to build/amiga/ if not already there
    bash "$PROJECT_DIR/scripts/install-emu.sh" 2>/dev/null

    # Copy UAEQuit to WORK: (always available regardless of system type)
    cp "$TOOLCHAIN_DIR/uaequit/UAEQuit" "$AMIGA_DIR/UAEQuit" 2>/dev/null || true

    # Copy ARexx test runner to build/amiga/ (mounted as WORK:)
    cp "$TOOLCHAIN_DIR/templates/test-runner.rexx" "$AMIGA_DIR/test-runner.rexx"

    # Generate test cases
    generate_test_cases "$port_dir"
    cp "$RESULTS_DIR/test-cases.txt" "$AMIGA_DIR/test-cases.txt"

    # Copy any test data files (test-*.txt) from the port directory to WORK:
    for datafile in "$port_dir"/test-*.txt; do
        if [ -f "$datafile" ] && [ "$(basename "$datafile")" != "test-fsemu-cases.txt" ]; then
            cp "$datafile" "$AMIGA_DIR/"
        fi
    done

    # Write User-Startup into the system
    # For HDF: inject via xdftool; for directory: write directly
    if [ -f "$SYSTEM_HDF" ]; then
        # Back up existing User-Startup from HDF
        xdftool "$SYSTEM_HDF" read S/User-Startup > "$RESULTS_DIR/User-Startup.bak" 2>/dev/null || true
    else
        mkdir -p "$SYSTEM_DIR/S"
        if [ -f "$SYSTEM_DIR/S/User-Startup" ]; then
            cp "$SYSTEM_DIR/S/User-Startup" "$SYSTEM_DIR/S/User-Startup.bak"
        fi
    fi

    # Build the User-Startup script content
    local startup_file="$RESULTS_DIR/User-Startup"

    if [ "$DEBUG_MODE" = true ]; then
        # Copy debug tools to WORK: for HDF mode
        local debug_tools_dir="$TOOLCHAIN_DIR/debug-tools"
        for tool in Enforcer MungWall SegTracker; do
            cp "$debug_tools_dir/$tool" "$AMIGA_DIR/$tool" 2>/dev/null || true
        done

        cat > "$startup_file" << 'AMIGA_SCRIPT'
; amiport FS-UAE test runner (DEBUG MODE)
Run >NIL: WORK:SegTracker
Run >NIL: WORK:MungWall
Run >NIL: WORK:Enforcer STACKLINES=4 STACKCHECK DATESTAMP
Wait 2
SYS:System/RexxMast >NIL:
Wait 2
SYS:Rexxc/rx WORK:test-runner.rexx
AMIGA_SCRIPT
    else
        cat > "$startup_file" << 'AMIGA_SCRIPT'
; amiport FS-UAE test runner
SYS:System/RexxMast >NIL:
Wait 2
SYS:Rexxc/rx WORK:test-runner.rexx
AMIGA_SCRIPT
    fi

    # Install User-Startup into the system
    if [ -f "$SYSTEM_HDF" ]; then
        # HDF mode: inject via xdftool
        # xdftool write fails with "Name already exists" if the file is present —
        # delete first to ensure a clean write regardless of prior test state.
        xdftool "$SYSTEM_HDF" delete S/User-Startup 2>/dev/null || true
        xdftool "$SYSTEM_HDF" write "$startup_file" S/User-Startup 2>/dev/null
    else
        # Directory mode: write directly
        cp "$startup_file" "$SYSTEM_DIR/S/User-Startup"
    fi
}

# ============================================================
# Generate FS-UAE config for this test run
# ============================================================
generate_config() {
    local config_file="$RESULTS_DIR/test-config.fs-uae"

    if [ -f "$SYSTEM_HDF" ]; then
        # Use HDF image (preferred — proper Workbench install)
        cat > "$config_file" << EOF
# FS-UAE config for automated testing (generated)
amiga_model = A1200
kickstart_file = $KICKSTART

# System hard drive (bootable Workbench 3.1 HDF)
hard_drive_0 = $SYSTEM_HDF

# Work drive (built ports and test harness)
hard_drive_1 = $AMIGA_DIR
hard_drive_1_label = WORK

# Results drive (test output written here)
hard_drive_2 = $RESULTS_DIR
hard_drive_2_label = RESULTS

# Headless settings
window_width = 720
window_height = 568
EOF
    else
        # Fallback: directory mount
        cat > "$config_file" << EOF
# FS-UAE config for automated testing (generated)
amiga_model = A1200
kickstart_file = $KICKSTART

# System hard drive (bootable Workbench 3.1 directory)
hard_drive_0 = $SYSTEM_DIR
hard_drive_0_label = System
hard_drive_0_priority = 1

# Work drive (built ports and test harness)
hard_drive_1 = $AMIGA_DIR
hard_drive_1_label = WORK

# Results drive (test output written here)
hard_drive_2 = $RESULTS_DIR
hard_drive_2_label = RESULTS

# Headless settings
window_width = 720
window_height = 568
EOF
    fi

    # Debug mode: add 68030 CPU, serial→TCP, JIT off, bsdsocket
    # FS-UAE LISTENS on SERIAL_PORT (server), serial-capture.py CONNECTS to it (client)
    if [ "$DEBUG_MODE" = true ]; then
        cat >> "$config_file" << EOF

# Debug mode settings (ADR-016)
cpu = 68030
serial_port = tcp://127.0.0.1:${SERIAL_PORT}/wait
jit_compiler = 0
bsdsocket_library = 1
EOF
    fi

    echo "$config_file"
}

# ============================================================
# Run FS-UAE with watchdog
# ============================================================
run_emulator() {
    local config_file="$1"
    local start_time
    start_time=$(date +%s)

    echo -e "${YELLOW}Launching FS-UAE headless (timeout: ${TIMEOUT_SECONDS}s)...${NC}"

    # Launch FS-UAE
    # Note: true headless mode (FSEMU_VIDEO_DRIVER=null) requires FS-UAE 4.0+
    # On FS-UAE 3.x, the window will appear briefly during the test run
    fs-uae "$config_file" >"$RESULTS_DIR/fsuae.log" 2>&1 &
    local fsuae_pid=$!

    # In debug mode, start serial capture client AFTER FS-UAE (FS-UAE is the server)
    # serial-capture.py retries connecting until FS-UAE starts listening
    if [ "$DEBUG_MODE" = true ]; then
        local ready_file="$RESULTS_DIR/.serial-ready"
        echo -e "${YELLOW}Connecting serial capture to FS-UAE on port ${SERIAL_PORT}...${NC}"
        python3 "$SCRIPT_DIR/serial-capture.py" "127.0.0.1:${SERIAL_PORT}" "$ENFORCER_LOG" --ready-file "$ready_file" &
        SERIAL_PID=$!

        # Wait for connection (serial-capture.py writes ready file when connected)
        local wait_count=0
        while [ ! -f "$ready_file" ]; do
            wait_count=$((wait_count + 1))
            if [ $wait_count -ge 100 ]; then  # 10s timeout
                echo -e "${YELLOW}  Serial capture still connecting (FS-UAE may be slow to start)...${NC}"
                # Don't abort — let the watchdog handle overall timeout
                break
            fi
            if ! kill -0 "$SERIAL_PID" 2>/dev/null; then
                echo -e "${RED}  Serial capture exited — FS-UAE may not support serial TCP${NC}"
                SERIAL_PID=""
                break
            fi
            sleep 0.1
        done
        if [ -f "$ready_file" ]; then
            echo "  Serial capture connected"
        fi
    fi

    # Watchdog loop
    while true; do
        local elapsed=$(( $(date +%s) - start_time ))

        # Check if FS-UAE exited (UAEQuit called)
        if ! kill -0 "$fsuae_pid" 2>/dev/null; then
            wait "$fsuae_pid" 2>/dev/null || true
            echo "  FS-UAE exited after ${elapsed}s"
            return 0
        fi

        # Check timeout
        if [ "$elapsed" -ge "$TIMEOUT_SECONDS" ]; then
            echo -e "${RED}  TIMEOUT: FS-UAE did not exit within ${TIMEOUT_SECONDS}s${NC}"
            kill "$fsuae_pid" 2>/dev/null || true
            wait "$fsuae_pid" 2>/dev/null || true
            return 1
        fi

        # Check for sentinel file (tests completed)
        if [ -f "$RESULTS_DIR/tests-complete" ]; then
            # Give FS-UAE a moment to process UAEQuit
            sleep 2
            if ! kill -0 "$fsuae_pid" 2>/dev/null; then
                wait "$fsuae_pid" 2>/dev/null || true
                echo "  Tests completed and FS-UAE exited after ${elapsed}s"
                return 0
            fi
        fi

        sleep 1
    done
}

# ============================================================
# Diagnose timeout causes
# ============================================================
diagnose_timeout() {
    local port_dir="$1"
    local elapsed="$2"
    local tap_file="$RESULTS_DIR/tap-output.txt"
    local sentinel="$RESULTS_DIR/tests-complete"

    echo ""
    echo "=== Timeout Diagnosis ==="

    # Check 1: Did ARexx harness start at all?
    if [ ! -f "$tap_file" ] || [ ! -s "$tap_file" ]; then
        echo -e "${RED}DIAGNOSIS: ARexx harness did not start${NC}"
        echo "  RexxMast may not be running, or test-runner.rexx failed to launch."
        return 1
    fi

    # Check 2: TAP header but no results?
    local has_header=$(grep -c '^1\.\.' "$tap_file" 2>/dev/null || echo 0)
    local result_count=$(grep -cE '^ok |^not ok ' "$tap_file" 2>/dev/null || echo 0)
    local total=$(head -1 "$tap_file" | grep -oE '[0-9]+$' || echo 0)

    if [ "$has_header" -gt 0 ] && [ "$result_count" -eq 0 ]; then
        if [ "$elapsed" -lt 15 ]; then
            echo -e "${RED}DIAGNOSIS: Binary crashed immediately (FS-UAE exited in ${elapsed}s)${NC}"
            echo "  The program likely segfaulted on startup."
            echo "  Run with --debug to capture Enforcer hits."
        else
            echo -e "${YELLOW}DIAGNOSIS: First test command hung (timeout after ${elapsed}s)${NC}"
            echo "  Likely cause: libnix exit() hang (crash-patterns #9)"
            echo "  The ARexx harness started ($total tests parsed) but the first"
            echo "  command never returned."
            check_exit_hang_pattern "$port_dir"
        fi
        return 1
    fi

    if [ "$has_header" -gt 0 ] && [ "$result_count" -gt 0 ] && [ "$result_count" -lt "$total" ]; then
        local next_test=$((result_count + 1))
        echo -e "${YELLOW}DIAGNOSIS: Test $next_test of $total hung${NC}"
        echo "  Tests 1-$result_count completed. Test $next_test never returned."
        return 1
    fi

    # Check 3: Tests completed but UAEQuit failed?
    if [ -f "$sentinel" ]; then
        echo -e "${YELLOW}DIAGNOSIS: Tests completed but UAEQuit failed${NC}"
        echo "  Results are valid — UAEQuit did not shut down FS-UAE."
        return 0
    fi

    echo -e "${RED}DIAGNOSIS: Unknown timeout cause${NC}"
    echo "  FS-UAE log: $RESULTS_DIR/fsuae.log"
    return 1
}

check_exit_hang_pattern() {
    local port_dir="$1"
    local ported_dir="$port_dir/ported"

    [ -d "$ported_dir" ] || return 1

    local has_exit=false
    local has__exit=false

    for src in "$ported_dir"/*.c; do
        [ -f "$src" ] || continue
        grep -q 'exit(' "$src" 2>/dev/null && has_exit=true
        grep -q '_exit(' "$src" 2>/dev/null && has__exit=true
    done

    if [ "$has_exit" = true ] && [ "$has__exit" = false ]; then
        echo ""
        echo -e "${YELLOW}  EXIT HANG PATTERN DETECTED:${NC}"
        echo "  Source has exit() but no _exit(). This matches crash-patterns #9."
        echo "  Fix: replace exit() at the end of main() with fflush(stdout); _exit();"
        return 2
    fi

    return 1
}

# ============================================================
# Parse TAP results
# ============================================================
parse_results() {
    local tap_file="$RESULTS_DIR/tap-output.txt"

    if [ ! -f "$tap_file" ]; then
        echo -e "${RED}ERROR: No test results found (tap-output.txt missing)${NC}"
        echo "  The ARexx test harness may not have run."
        echo "  Check if RexxMast is started in S/Startup-Sequence."
        return 1
    fi

    echo ""
    echo "=== FS-UAE Test Results ==="
    cat "$tap_file"
    echo ""

    # Parse pass/fail counts
    local total passed failed
    total=$(grep -cE "^ok |^not ok" "$tap_file" 2>/dev/null) || total=0
    passed=$(grep -c "^ok " "$tap_file" 2>/dev/null) || passed=0
    failed=$(grep -c "^not ok" "$tap_file" 2>/dev/null) || failed=0

    if [ "$failed" -gt 0 ]; then
        echo -e "${RED}RESULT: $passed/$total passed, $failed FAILED${NC}"
        return 1
    else
        echo -e "${GREEN}RESULT: $passed/$total passed${NC}"
        return 0
    fi
}

# ============================================================
# Generate test report
# ============================================================
generate_report() {
    local port_dir="$1"
    local port_name
    port_name=$(basename "$port_dir")
    local report_file="$port_dir/TEST-REPORT.md"
    local template="$TOOLCHAIN_DIR/templates/test-report.md.template"
    local tap_file="$RESULTS_DIR/tap-output.txt"
    local sentinel="$RESULTS_DIR/tests-complete"
    local cases_file="$port_dir/test-fsemu-cases.txt"

    if [ ! -f "$template" ]; then
        echo -e "${YELLOW}No report template found — skipping report generation${NC}"
        return 0
    fi

    # Gather data
    local fsuae_version
    fsuae_version=$(fs-uae --version 2>/dev/null || echo "unknown")
    local binary_size="unknown"
    if [ -f "$port_dir/$port_name" ]; then
        binary_size=$(ls -lh "$port_dir/$port_name" | awk '{print $5}')
    fi
    local duration=$(($(date +%s) - START_TIME))
    local date_str
    date_str=$(date "+%Y-%m-%d %H:%M:%S")

    # Determine result
    local result_str="UNKNOWN"
    local passed=0 failed=0 total=0
    if [ -f "$tap_file" ]; then
        total=$(grep -cE "^ok |^not ok" "$tap_file" 2>/dev/null) || total=0
        passed=$(grep -c "^ok " "$tap_file" 2>/dev/null) || passed=0
        failed=$(grep -c "^not ok" "$tap_file" 2>/dev/null) || failed=0
        if [ "$failed" -gt 0 ]; then
            result_str="**FAIL** — $passed/$total passed, $failed failed"
        else
            result_str="**PASS** — $passed/$total passed"
        fi
    else
        result_str="**NO RESULTS** — ARexx harness did not produce output"
    fi

    # Build test table from TAP output
    local test_table=""
    local test_num=0
    if [ -f "$tap_file" ]; then
        while IFS= read -r line; do
            case "$line" in
                "ok "*)
                    test_num=$((test_num + 1))
                    local desc="${line#ok * - }"
                    [ "$desc" = "$line" ] && desc="${line#ok *}"
                    test_table="${test_table}| $test_num | $desc | PASS | |"$'\n'
                    ;;
                "not ok "*)
                    test_num=$((test_num + 1))
                    local desc="${line#not ok * - }"
                    [ "$desc" = "$line" ] && desc="${line#not ok *}"
                    test_table="${test_table}| $test_num | $desc | **FAIL** | |"$'\n'
                    ;;
                "#   expected:"*|"#   actual:"*)
                    # Append detail to previous row
                    test_table="${test_table}| | | | \`${line#\#   }\` |"$'\n'
                    ;;
            esac
        done < "$tap_file"
    fi

    # Read raw inputs
    local tap_output="(no results)"
    [ -f "$tap_file" ] && tap_output=$(cat "$tap_file")

    local test_cases_raw="(no test cases file)"
    [ -f "$cases_file" ] && test_cases_raw=$(cat "$cases_file")

    local sentinel_contents="(not created — tests may not have completed)"
    [ -f "$sentinel" ] && sentinel_contents=$(cat "$sentinel")

    local emu_log="(log not captured in this run)"

    # Generate report from template
    sed -e "s|__PORT_NAME__|$port_name|g" \
        -e "s|__DATE__|$date_str|g" \
        -e "s|__DURATION__|$duration|g" \
        -e "s|__FSUAE_VERSION__|$fsuae_version|g" \
        -e "s|__BINARY_SIZE__|$binary_size|g" \
        -e "s|__RESULT__|$result_str|g" \
        "$template" > "$report_file"

    # Replace multi-line placeholders via temp files (safe for arbitrary content)
    printf '%s\n' "$tap_output" > "$RESULTS_DIR/.tap_output"
    printf '%s\n' "$test_cases_raw" > "$RESULTS_DIR/.test_cases_raw"
    printf '%s\n' "$sentinel_contents" > "$RESULTS_DIR/.sentinel_contents"
    printf '%s\n' "$emu_log" > "$RESULTS_DIR/.emu_log"
    printf '%s\n' "$test_table" > "$RESULTS_DIR/.test_table"

    python3 -c "
import sys, os
d = sys.argv[1]
with open(sys.argv[2], 'r') as f:
    content = f.read()
for key, fname in [('__TAP_OUTPUT__', '.tap_output'), ('__TEST_CASES_RAW__', '.test_cases_raw'),
                    ('__SENTINEL_CONTENTS__', '.sentinel_contents'), ('__EMU_LOG__', '.emu_log'),
                    ('__TEST_TABLE__', '.test_table')]:
    path = os.path.join(d, fname)
    if os.path.exists(path):
        with open(path) as vf:
            content = content.replace(key, vf.read().rstrip())
with open(sys.argv[2], 'w') as f:
    f.write(content)
" "$RESULTS_DIR" "$report_file"

    echo ""
    echo -e "${GREEN}Test report saved to: $report_file${NC}"
    echo ""
}

# ============================================================
# Cleanup
# ============================================================
cleanup() {
    # Kill serial capture daemon if still running
    if [ -n "$SERIAL_PID" ] && kill -0 "$SERIAL_PID" 2>/dev/null; then
        kill "$SERIAL_PID" 2>/dev/null || true
        wait "$SERIAL_PID" 2>/dev/null || true
    fi

    # Restore original User-Startup
    if [ -f "$SYSTEM_HDF" ]; then
        # HDF mode: restore via xdftool
        # Delete before write — xdftool write fails with "Name already exists" if the
        # file is present (e.g., after a failed/interrupted previous run).
        if [ -n "${RESULTS_DIR:-}" ] && [ -f "$RESULTS_DIR/User-Startup.bak" ]; then
            xdftool "$SYSTEM_HDF" delete S/User-Startup 2>/dev/null || true
            xdftool "$SYSTEM_HDF" write "$RESULTS_DIR/User-Startup.bak" S/User-Startup 2>/dev/null || true
        else
            xdftool "$SYSTEM_HDF" delete S/User-Startup 2>/dev/null || true
        fi
    elif [ -f "$SYSTEM_DIR/S/User-Startup.bak" ]; then
        mv "$SYSTEM_DIR/S/User-Startup.bak" "$SYSTEM_DIR/S/User-Startup"
    else
        rm -f "$SYSTEM_DIR/S/User-Startup"
    fi

    # Clean up temp files from WORK:
    rm -f "$AMIGA_DIR/test-runner.rexx" "$AMIGA_DIR/test-cases.txt"

    # Clean up results dir
    if [ -n "${RESULTS_DIR:-}" ] && [ -d "${RESULTS_DIR:-}" ]; then
        rm -rf "$RESULTS_DIR"
    fi
}

# ============================================================
# Main
# ============================================================
main() {
    local port_dir=""

    local timeout_set=false

    # Parse arguments
    while [ $# -gt 0 ]; do
        case "$1" in
            --debug)
                DEBUG_MODE=true
                shift
                ;;
            --timeout)
                TIMEOUT_SECONDS="$2"
                timeout_set=true
                shift 2
                ;;
            --all)
                echo "TODO: --all mode not yet implemented"
                exit 1
                ;;
            -h|--help)
                usage
                ;;
            *)
                port_dir="$1"
                shift
                ;;
        esac
    done

    # Debug mode defaults to 90s timeout (3x normal) unless explicitly set
    if [ "$DEBUG_MODE" = true ] && [ "$timeout_set" = false ]; then
        TIMEOUT_SECONDS=90
    fi

    if [ -z "$port_dir" ]; then
        usage
    fi

    # Resolve port directory
    port_dir="$PROJECT_DIR/$port_dir"
    if [ ! -d "$port_dir" ]; then
        echo -e "${RED}ERROR: Port directory not found: $port_dir${NC}"
        exit 1
    fi

    local port_name
    port_name=$(basename "$port_dir")

    echo "=== FS-UAE Testing: $port_name ==="

    START_TIME=$(date +%s)

    # Set up trap for cleanup
    trap cleanup EXIT

    # Pre-flight checks
    preflight

    # Create results directory early (needed by serial capture for sentinel file)
    RESULTS_DIR=$(mktemp -d "${TMPDIR:-/tmp}/amiport-fsemu-XXXXXX")

    # Debug mode setup: pick a serial port for FS-UAE config
    # FS-UAE listens on this port (server), serial-capture.py connects to it (client)
    if [ "$DEBUG_MODE" = true ]; then
        ENFORCER_LOG="$port_dir/enforcer.log"
        # Use a port in the ephemeral range, offset by PID to avoid conflicts
        SERIAL_PORT=$(( 10000 + ($$ % 50000) ))
        echo "$SERIAL_PORT" > "$RESULTS_DIR/.serial-port"
    fi

    # Build boot volume
    build_boot_volume "$port_dir"

    # Generate config
    local config_file
    config_file=$(generate_config)

    # Run emulator with watchdog
    local emu_exit=0
    run_emulator "$config_file" || emu_exit=$?

    # Stop serial capture daemon
    if [ "$DEBUG_MODE" = true ] && [ -n "$SERIAL_PID" ]; then
        kill "$SERIAL_PID" 2>/dev/null || true
        wait "$SERIAL_PID" 2>/dev/null || true
        SERIAL_PID=""
    fi

    # Analyze Enforcer log (debug mode)
    local enforcer_hits=0
    if [ "$DEBUG_MODE" = true ] && [ -f "$ENFORCER_LOG" ] && [ -s "$ENFORCER_LOG" ]; then
        echo ""
        echo "=== Enforcer Crash Analysis ==="

        # Parse Enforcer hits
        local hits_json="$port_dir/enforcer-hits.json"
        if python3 "$SCRIPT_DIR/debug-report.py" parse "$ENFORCER_LOG" --app "$port_name" > "$hits_json" 2>/dev/null; then
            enforcer_hits=$(python3 -c "import json,sys; d=json.load(open('$hits_json')); print(d['summary']['total_hits'])" 2>/dev/null || echo "0")

            if [ "$enforcer_hits" -gt 0 ]; then
                echo -e "${RED}  $enforcer_hits Enforcer hit(s) detected!${NC}"

                # Map addresses to source lines
                local port_name
                port_name=$(basename "$port_dir")
                local binary="$port_dir/$port_name"
                if [ -f "$binary" ]; then
                    local mapped_json="$port_dir/enforcer-mapped.json"
                    python3 "$SCRIPT_DIR/debug-report.py" map "$hits_json" "$binary" > "$mapped_json" 2>/dev/null || true
                    if [ -f "$mapped_json" ] && [ -s "$mapped_json" ]; then
                        echo "  Source mapping saved to: $mapped_json"
                    fi
                fi

                # Summary
                python3 -c "
import json, sys
try:
    d = json.load(open('$hits_json'))
    for ct in d['summary'].get('crash_types', []):
        print(f'  Type: {ct}')
except: pass
" 2>/dev/null || true
            else
                echo -e "${GREEN}  No Enforcer hits — clean run${NC}"
            fi
        else
            echo -e "${YELLOW}  Warning: Failed to parse Enforcer log${NC}"
        fi
    elif [ "$DEBUG_MODE" = true ]; then
        echo ""
        echo "=== Enforcer Crash Analysis ==="
        # Check for hard crash (no TAP results AND no Enforcer output)
        if [ ! -f "$RESULTS_DIR/tests-complete" ] && [ $emu_exit -ne 0 ]; then
            echo -e "${RED}  HARD CRASH detected — no Enforcer output, no test completion${NC}"
            echo "  This may be a Guru Meditation (bus error, illegal instruction)."
            echo "  Use /debug-amiga to insert printf breadcrumbs and isolate the crash."
        else
            echo -e "${GREEN}  No Enforcer output (clean run or Enforcer not active)${NC}"
        fi
    fi

    # Check if tests completed (sentinel file exists) regardless of
    # how the emulator exited. UAEQuit may not work in headless mode,
    # so the watchdog kills FS-UAE after timeout — but if the sentinel
    # file exists, the tests DID complete successfully.
    if [ -f "$RESULTS_DIR/tests-complete" ]; then
        parse_results
        local test_exit=$?
        generate_report "$port_dir"
        exit $test_exit
    elif [ $emu_exit -ne 0 ]; then
        local elapsed=$(( $(date +%s) - START_TIME ))
        diagnose_timeout "$port_dir" "$elapsed"
        parse_results 2>/dev/null || true
        generate_report "$port_dir"
        exit 1
    else
        parse_results
        local test_exit=$?
        generate_report "$port_dir"
        exit $test_exit
    fi
}

main "$@"
