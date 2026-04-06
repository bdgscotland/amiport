#!/bin/bash
# amiport status line — shows model, context window %, rate limits, published count.

input=$(cat)
[ -z "$input" ] && { echo "amiport"; exit 0; }

PROJECT_ROOT="/Users/duncan/Developer/amiport"

# Colors (bash $'...' for real escape bytes)
R=$'\033[0m' DIM=$'\033[90m' GRN=$'\033[32m' AMB=$'\033[33m' RED=$'\033[31m'

# ---------------------------------------------------------------------------
# Extract fields from JSON (single jq call, tab-separated)
# ---------------------------------------------------------------------------
IFS=$'\t' read -r model used_pct input_tokens max_window five_pct week_pct <<< \
  "$(echo "$input" | jq -r '[
    (.model.display_name // ""),
    (.context_window.used_percentage // ""),
    (.context_window.current_usage.input_tokens // 0),
    (.context_window.context_window_size // 0),
    (.rate_limits.five_hour.used_percentage // ""),
    (.rate_limits.seven_day.used_percentage // "")
  ] | join("\t")' 2>/dev/null)"

# ---------------------------------------------------------------------------
# Context window usage
# ---------------------------------------------------------------------------
if [ -z "$used_pct" ] || [ "$used_pct" = "null" ]; then
  if [ "$max_window" -gt 0 ] 2>/dev/null && [ "$input_tokens" -gt 0 ] 2>/dev/null; then
    used_pct=$(( input_tokens * 100 / max_window ))
  fi
fi

ctx=""
if [ -n "$used_pct" ] && [ "$used_pct" != "null" ] && [ "$used_pct" != "0" ]; then
  pct=$(printf "%.0f" "$used_pct" 2>/dev/null || echo "$used_pct")
  if [ "$pct" -ge 80 ] 2>/dev/null; then c="$RED"
  elif [ "$pct" -ge 50 ] 2>/dev/null; then c="$AMB"
  else c="$GRN"; fi
  ctx=" ${c}ctx:${pct}%${R}"
fi

# ---------------------------------------------------------------------------
# Rate limits — show when >50%
# ---------------------------------------------------------------------------
rates=""
if [ -n "$five_pct" ] && [ "$five_pct" != "null" ]; then
  fi=$(printf "%.0f" "$five_pct" 2>/dev/null || echo "$five_pct")
  [ "$fi" -ge 50 ] 2>/dev/null && rates="${rates} ${AMB}5h:${fi}%${R}"
fi
if [ -n "$week_pct" ] && [ "$week_pct" != "null" ]; then
  wi=$(printf "%.0f" "$week_pct" 2>/dev/null || echo "$week_pct")
  [ "$wi" -ge 50 ] 2>/dev/null && rates="${rates} ${AMB}7d:${wi}%${R}"
fi

# ---------------------------------------------------------------------------
# Published port count
# ---------------------------------------------------------------------------
pub=0
if [ -f "$PROJECT_ROOT/data/catalog.json" ]; then
  n=$(jq '[.packages // {} | to_entries[] | select(.value.status == "published")] | length' \
    "$PROJECT_ROOT/data/catalog.json" 2>/dev/null)
  [ -n "$n" ] && [ "$n" -gt 0 ] 2>/dev/null && pub=$n
fi

# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------
line=""
[ -n "$model" ] && [ "$model" != "null" ] && line="${DIM}${model}${R}"
[ -n "$ctx" ] && line="${line}${ctx}"
[ -n "$rates" ] && line="${line}${rates}"
[ "$pub" -gt 0 ] 2>/dev/null && line="${line} ${DIM}| ${pub} shipped${R}"

[ -n "$line" ] && printf "%s\n" "$line"
