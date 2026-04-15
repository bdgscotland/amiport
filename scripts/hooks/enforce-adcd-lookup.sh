#!/bin/bash
# Hook: enforce-adcd-lookup.sh
# PreToolUse hook for Edit|Write — warns when modifying C files that
# contain AmigaOS or library API includes without having loaded the
# relevant reference docs / queried the KB.
#
# Two warning bands:
#   1. AmigaOS API headers (proto/*, devices/*, exec/*, intuition/*,
#      graphics/*, libraries/*, dos/*)
#      -> MUST invoke /amiga-api-lookup + dispatch hardware-expert
#   2. Library consumer headers (git2.h / libgit2/*, amiport-net/*,
#      bsdsocket/*, amissl/*, http_client.h / pkt_line.h / amissl_glue.h
#      inside amigit)
#      -> MUST query amiga-kb via amiga_pitfalls_for BEFORE the first
#         Edit/Write on the file
#
# Both are WARN hooks (exits 0 with message), not BLOCK hooks. The
# agent sees the warning and should act on it. This exists because
# at least one session has shipped Phase 5-style code touching
# libgit2 / bsdsocket / AmiSSL consumers without ever querying the
# KB, only catching the miss after the user had to interrupt
# mid-session to remind them ("you have a kb and you have arexx and
# so on"). The KB-query-before-edit discipline is not optional.

FILE_PATH="$TOOL_INPUT_FILE_PATH"

# Only check C source files
case "$FILE_PATH" in
    *.c|*.h) ;;
    *) exit 0 ;;
esac

# Skip if file doesn't exist yet (new file creation)
[ -f "$FILE_PATH" ] || exit 0

# Band 1: AmigaOS API headers -> ADCD lookup + hardware-expert
if grep -qE '#include\s*<(proto|devices|exec|intuition|graphics|libraries|dos)/' "$FILE_PATH" 2>/dev/null; then
    echo "WARNING: This file includes AmigaOS API headers."
    echo "MANDATORY: Invoke /amiga-api-lookup before writing AmigaOS API code."
    echo "MANDATORY: Dispatch hardware-expert agent for hardware assumptions."
    echo "Do NOT guess at struct layouts, function signatures, or field offsets."
fi

# Band 2: Library consumer headers -> amiga-kb pitfall query
# This catches libgit2 consumers (git2.h / lib/libgit2 headers),
# bsdsocket consumers (amiport-net/*), AmiSSL consumers (amissl/*),
# and the amigit-internal http_client / pkt_line / amissl_glue
# layer that sits on top of them.
if grep -qE '#include\s*[<"](git2\.h|git2/|amiport-net/|amissl/|http_client\.h|pkt_line\.h|amissl_glue\.h|amigit\.h)' "$FILE_PATH" 2>/dev/null; then
    echo "WARNING: This file includes a library consumer header (libgit2 / bsdsocket / AmiSSL / amigit http layer)."
    echo "MANDATORY: BEFORE editing, query amiga-kb pitfalls via MCP:"
    echo "  amiga_pitfalls_for  topic=\"<what this TU consumes>\""
    echo "  amiga_search        query=\"<relevant 68k / libnix / bebbo concern>\""
    echo "At minimum, query once per session per TU. The KB catches known"
    echo "libgit2 codegen crashes (patch_generate softfloat), git_transport_register"
    echo "bare-scheme, bsdsocket fd collisions, AmiSSL OpenAmiSSLTags vs legacy,"
    echo "and other load-bearing pitfalls. Skipping the query has shipped bugs."
fi

exit 0
