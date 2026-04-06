#!/bin/bash

# Find all malloc/xmalloc/xstrdup allocations and check for proper cleanup

echo "=== CRITICAL ALLOCATION FUNCTIONS ==="
grep -rn "xmalloc\|xcalloc\|xrealloc\|xstrdup\|aprintf\|strdup\|malloc\|calloc\|realloc" ported/src/*.c | grep -v "//.*" | wc -l
echo " allocation patterns found"

echo ""
echo "=== NO ATEXIT CLEANUP REGISTERED ===" 
grep -rn "atexit\|amiport_free_argv" ported/src/main.c 

echo ""
echo "=== EXIT CALLS IN MAIN ===" 
grep -n "exit(" ported/src/main.c | head -20

echo ""
echo "=== getenv USAGE (CRITICAL FOR AMIGA) ===" 
grep -n "getenv(" ported/src/*.c | head -20

echo ""
echo "=== CLEANUP FUNCTION GATING ===" 
grep -A5 "#if defined DEBUG_MALLOC\|#if defined TESTING" ported/src/init.c | head -20

echo ""
echo "=== CRITICAL: home_dir() ===" 
grep -A20 "^home_dir" ported/src/init.c | head -25

