#!/bin/bash
#
# inject-keys.sh — Send keystrokes to FS-UAE window from macOS host
#
# Converts the same comma-separated key sequence format used by KeyInject
# into osascript key events. This injects through FS-UAE's SDL input path
# (same as physical keypresses), unlike KeyInject's AddIEvents() which
# doesn't reliably deliver to the focused Amiga window.
#
# Usage: inject-keys.sh <fs-uae-pid> <key-sequence>
#   inject-keys.sh 12345 "WAIT2000,CTRL_N,WAIT1000"
#   inject-keys.sh 12345 "DOWN,DOWN,DOWN"
#
# Architecture: ADR-025

set -euo pipefail

FSUAE_PID="${1:?Usage: inject-keys.sh <fs-uae-pid> <key-sequence>}"
KEYS="${2:?Usage: inject-keys.sh <fs-uae-pid> <key-sequence>}"

# macOS key codes for special keys
# Reference: Events.h / Carbon HIToolbox
send_key() {
    local keycode="$1"
    local modifiers="${2:-}"

    if [ -n "$modifiers" ]; then
        osascript -e "
            tell application \"System Events\"
                tell process \"fs-uae\"
                    set frontmost to true
                    key code $keycode using $modifiers
                end tell
            end tell
        " 2>/dev/null
    else
        osascript -e "
            tell application \"System Events\"
                tell process \"fs-uae\"
                    set frontmost to true
                    key code $keycode
                end tell
            end tell
        " 2>/dev/null
    fi
}

send_char() {
    local char="$1"
    local modifiers="${2:-}"

    if [ -n "$modifiers" ]; then
        osascript -e "
            tell application \"System Events\"
                tell process \"fs-uae\"
                    set frontmost to true
                    keystroke \"$char\" using $modifiers
                end tell
            end tell
        " 2>/dev/null
    else
        osascript -e "
            tell application \"System Events\"
                tell process \"fs-uae\"
                    set frontmost to true
                    keystroke \"$char\"
                end tell
            end tell
        " 2>/dev/null
    fi
}

# Process comma-separated token sequence
IFS=',' read -ra TOKENS <<< "$KEYS"

for token in "${TOKENS[@]}"; do
    token=$(echo "$token" | tr -d ' ')  # trim whitespace

    case "$token" in
        WAIT[0-9]*)
            ms="${token#WAIT}"
            # Convert ms to seconds for sleep (macOS sleep supports decimals)
            sleep "$(echo "scale=3; $ms / 1000" | bc)"
            ;;
        UP)      send_key 126 ;;
        DOWN)    send_key 125 ;;
        LEFT)    send_key 123 ;;
        RIGHT)   send_key 124 ;;
        RETURN)  send_key 36 ;;
        ESC)     send_key 53 ;;
        SPACE)   send_key 49 ;;
        TAB)     send_key 48 ;;
        BACKSPACE) send_key 51 ;;
        DELETE)  send_key 117 ;;
        F1)      send_key 122 ;;
        F2)      send_key 120 ;;
        F3)      send_key 99 ;;
        F4)      send_key 118 ;;
        F5)      send_key 96 ;;
        F6)      send_key 97 ;;
        F7)      send_key 98 ;;
        F8)      send_key 100 ;;
        F9)      send_key 101 ;;
        F10)     send_key 109 ;;
        HELP)    send_key 114 ;;  # Insert/Help key
        CTRL_*)
            key="${token#CTRL_}"
            case "$key" in
                [A-Z]) send_char "$(echo "$key" | tr 'A-Z' 'a-z')" "control down" ;;
                [a-z]) send_char "$key" "control down" ;;
                SPACE) send_key 49 "control down" ;;
                SLASH) send_char "/" "control down" ;;
                *)     echo "inject-keys: unknown CTRL_ target: $key" >&2 ;;
            esac
            ;;
        ALT_*)
            key="${token#ALT_}"
            case "$key" in
                [A-Z]) send_char "$(echo "$key" | tr 'A-Z' 'a-z')" "option down" ;;
                [a-z]) send_char "$key" "option down" ;;
                *)     echo "inject-keys: unknown ALT_ target: $key" >&2 ;;
            esac
            ;;
        ?)
            # Single character
            send_char "$token"
            ;;
        *)
            echo "inject-keys: unknown token: $token" >&2
            ;;
    esac

    # Small delay between keystrokes (50ms)
    sleep 0.05
done
