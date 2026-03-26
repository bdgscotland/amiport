#!/bin/sh
# amiport status line — pipeline-aware status for the Amiga porting project.
# Reads JSON from stdin (Claude Code statusLine protocol).

input=$(cat)

PROJECT_ROOT="/Users/duncan/Developer/amiport"

# ---------------------------------------------------------------------------
# Extract core fields from JSON input
# ---------------------------------------------------------------------------
cwd=$(echo "$input" | jq -r '.workspace.current_dir // .cwd // ""')
model=$(echo "$input" | jq -r '.model.display_name // ""')

# Worktree detection — batch-port-parallel uses worktrees per port
worktree_name=$(echo "$input" | jq -r '.worktree.name // ""')
worktree_branch=$(echo "$input" | jq -r '.worktree.branch // ""')

# Agent detection — port-worker or named pipeline agents
agent_name=$(echo "$input" | jq -r '.agent.name // ""')

# Session name (set via /rename for parallel sessions)
session_name=$(echo "$input" | jq -r '.session_name // ""')

# ---------------------------------------------------------------------------
# Context window usage
# ---------------------------------------------------------------------------
used_pct=$(echo "$input" | jq -r '.context_window.used_percentage // empty')
if [ -n "$used_pct" ]; then
  ctx_label=$(printf "%.0f%%" "$used_pct")
  # Colour-code by pressure: green < 50, amber 50-80, red > 80
  used_int=$(printf "%.0f" "$used_pct")
  if [ "$used_int" -ge 80 ]; then
    ctx_color="\033[31m"   # red
  elif [ "$used_int" -ge 50 ]; then
    ctx_color="\033[33m"   # amber
  else
    ctx_color="\033[32m"   # green
  fi
  ctx_part=" ${ctx_color}ctx:${ctx_label}\033[0m"
else
  ctx_part=""
fi

# ---------------------------------------------------------------------------
# Rate limits (Claude.ai subscription) — show when >50% used
# ---------------------------------------------------------------------------
rate_part=""
five_pct=$(echo "$input" | jq -r '.rate_limits.five_hour.used_percentage // empty')
week_pct=$(echo "$input" | jq -r '.rate_limits.seven_day.used_percentage // empty')
if [ -n "$five_pct" ]; then
  five_int=$(printf "%.0f" "$five_pct")
  if [ "$five_int" -ge 50 ]; then
    rate_part="${rate_part} \033[33m5h:${five_int}%\033[0m"
  fi
fi
if [ -n "$week_pct" ]; then
  week_int=$(printf "%.0f" "$week_pct")
  if [ "$week_int" -ge 50 ]; then
    rate_part="${rate_part} \033[33m7d:${week_int}%\033[0m"
  fi
fi

# ---------------------------------------------------------------------------
# Detect active ports — scan ports/ for ported/ subdirectories
# ---------------------------------------------------------------------------
active_ports=""
port_count=0
if [ -d "$PROJECT_ROOT/ports" ]; then
  for port_dir in "$PROJECT_ROOT"/ports/*/; do
    [ -d "${port_dir}ported" ] || continue
    pname=$(basename "$port_dir")
    [ "$pname" = "templates" ] && continue
    port_count=$((port_count + 1))
    active_ports="${active_ports}${pname} "
  done
fi
active_ports="${active_ports% }"  # strip trailing space

# ---------------------------------------------------------------------------
# Infer pipeline stage from the current working directory and recent activity
# ---------------------------------------------------------------------------
# Strategy: look at what subdirectory of ports/<name>/ we are in, or what
# files are newest, to hint at the current stage.
pipeline_stage=""
focused_port=""

# If cwd is inside a port directory, extract port name
case "$cwd" in
  "$PROJECT_ROOT"/ports/*)
    # e.g. /Users/duncan/Developer/amiport/ports/basename/ported
    remainder="${cwd#$PROJECT_ROOT/ports/}"
    focused_port="${remainder%%/*}"
    subdir="${remainder#*/}"
    case "$subdir" in
      original*)   pipeline_stage="Stage 1: Analyze" ;;
      ported*)     pipeline_stage="Stage 3: Transform" ;;
      *)           pipeline_stage="" ;;
    esac
    ;;
  "$PROJECT_ROOT"/lib/*)
    pipeline_stage="Shim dev" ;;
  "$PROJECT_ROOT"/site/*)
    pipeline_stage="Site" ;;
  "$PROJECT_ROOT"/tests/*)
    pipeline_stage="Tests" ;;
esac

# Worktree overrides focused_port — each worktree IS a port
if [ -n "$worktree_name" ]; then
  focused_port="$worktree_name"
  if [ -n "$worktree_branch" ] && [ "$worktree_branch" != "$worktree_name" ]; then
    focused_port="${worktree_name} (${worktree_branch})"
  fi
fi

# Agent name gives a strong stage signal
case "$agent_name" in
  aminet-researcher)    pipeline_stage="Stage 0: Research" ;;
  dependency-auditor)   pipeline_stage="Stage 0b: Dep audit" ;;
  source-analyzer)      pipeline_stage="Stage 1: Analyze" ;;
  code-transformer)     pipeline_stage="Stage 3: Transform" ;;
  build-manager)        pipeline_stage="Stage 4: Build" ;;
  test-designer)        pipeline_stage="Stage 5b: Test design" ;;
  test-runner)          pipeline_stage="Stage 5: vamos test" ;;
  memory-checker)       pipeline_stage="Stage 6b: Memory audit" ;;
  perf-optimizer)       pipeline_stage="Stage 6c: Perf review" ;;
  profiler)             pipeline_stage="Stage 6d: Profile" ;;
  debug-agent)          pipeline_stage="Debug" ;;
  visual-test-expert)   pipeline_stage="Visual tests" ;;
  amiport-publisher)    pipeline_stage="Publish" ;;
  aminet-publisher)     pipeline_stage="Aminet publish" ;;
  site-manager)         pipeline_stage="Site deploy" ;;
  port-worker)          pipeline_stage="Batch port" ;;
  catalog-engineer)     pipeline_stage="Catalog" ;;
  hardware-expert)      pipeline_stage="HW review" ;;
esac

# Session name overrides stage label when set explicitly
if [ -n "$session_name" ]; then
  pipeline_stage="$session_name"
fi

# ---------------------------------------------------------------------------
# Count total published ports from catalog (fast: count entries with lha)
# ---------------------------------------------------------------------------
published_count=0
if [ -f "$PROJECT_ROOT/data/catalog.json" ]; then
  lha_count=$(jq -r '.packages // {} | to_entries[] | select(.value.status == "published") | .key' \
    "$PROJECT_ROOT/data/catalog.json" 2>/dev/null | wc -l | tr -d ' ')
  [ -n "$lha_count" ] && published_count=$lha_count
fi

# Fallback: count .lha files in ports/
if [ "$published_count" -eq 0 ]; then
  published_count=$(find "$PROJECT_ROOT/ports" -maxdepth 2 -name "*.lha" 2>/dev/null | wc -l | tr -d ' ')
fi

# ---------------------------------------------------------------------------
# Assemble the status line
# ---------------------------------------------------------------------------

# Line 1: Port focus + pipeline stage (the most important line)
if [ -n "$focused_port" ] && [ -n "$pipeline_stage" ]; then
  printf "\033[36mAmiga port:\033[0m \033[1m%s\033[0m \033[90m— %s\033[0m\n" \
    "$focused_port" "$pipeline_stage"
elif [ -n "$focused_port" ]; then
  printf "\033[36mAmiga port:\033[0m \033[1m%s\033[0m\n" "$focused_port"
elif [ -n "$pipeline_stage" ]; then
  printf "\033[36mAmiport:\033[0m \033[90m%s\033[0m\n" "$pipeline_stage"
elif [ "$port_count" -gt 0 ]; then
  # Show active port list (truncated if many)
  if [ "$port_count" -le 4 ]; then
    printf "\033[36mPorts:\033[0m %s\n" "$active_ports"
  else
    first4=$(echo "$active_ports" | tr ' ' '\n' | head -4 | tr '\n' ' ')
    rest=$((port_count - 4))
    printf "\033[36mPorts:\033[0m %s\033[90m+%d more\033[0m\n" "$first4" "$rest"
  fi
fi

# Line 2: Model + context + rate limits + published count
meta_line=""
if [ -n "$model" ]; then
  meta_line="\033[90m${model}\033[0m"
fi
if [ -n "$ctx_part" ]; then
  meta_line="${meta_line}${ctx_part}"
fi
if [ -n "$rate_part" ]; then
  meta_line="${meta_line}${rate_part}"
fi
if [ "$published_count" -gt 0 ]; then
  meta_line="${meta_line} \033[90m| ${published_count} published\033[0m"
fi

if [ -n "$meta_line" ]; then
  printf "${meta_line}\n"
fi
