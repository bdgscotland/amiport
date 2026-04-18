#!/bin/bash
# Autonomous iterate loop: rebuild, deploy, launch FS-UAE, poll, report, kill.
# Runs ONE iteration. Re-run to iterate again.
set -e
cd "$(dirname "$0")/../../.."

echo "=== [1/5] Rebuild SDL2 GUI binary ==="
docker run --rm \
    -v $(pwd):/amiport \
    -v /Users/duncan/Developer/libSDL2-amigaos3:/sdl2 \
    -v /tmp:/tmp \
    ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    bash /amiport/ports/openttd/debug-wip/recompile_openttd_sdl2.sh 2>&1 | tail -3

echo ""
echo "=== [2/5] Strip + deploy ==="
docker run --rm -v /tmp:/tmp ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest \
    /opt/amiga13/bin/m68k-amigaos-strip --strip-debug /tmp/openttd-sdl2
cp /tmp/openttd-sdl2 build/amiga/OpenTTD-SDL2/openttd-sdl2
ls -la build/amiga/OpenTTD-SDL2/openttd-sdl2

echo ""
echo "=== [3/5] Clear logs + launch FS-UAE in background ==="
pkill -f fs-uae 2>/dev/null || true
sleep 2
rm -f build/amiga/OpenTTD-SDL2/run.log build/amiga/OpenTTD-SDL2/debug.log \
      build/amiga/OpenTTD-SDL2/ctor-1.txt build/amiga/OpenTTD-SDL2/ctor-2.txt \
      ~/Documents/FS-UAE/Cache/Logs/fs-uae.log.txt
fs-uae ports/openttd/openttd-test.fs-uae > /tmp/fs-uae-iter.stdout.log 2> /tmp/fs-uae-iter.stderr.log &
FSPID=$!
echo "FS-UAE PID=$FSPID"

echo ""
echo "=== [4/5] Wait 90s for boot + run ==="
for i in 30 60 90; do
    sleep 30
    echo "  T+${i}s: ctors=$(ls build/amiga/OpenTTD-SDL2/ctor-*.txt 2>/dev/null | wc -l | tr -d ' ') run.log=$(wc -l < build/amiga/OpenTTD-SDL2/run.log 2>/dev/null | tr -d ' ') debug.log=$(wc -l < build/amiga/OpenTTD-SDL2/debug.log 2>/dev/null | tr -d ' ')"
done

echo ""
echo "=== [5/5] Report + kill ==="
echo "--- ctor-1.txt:"; cat build/amiga/OpenTTD-SDL2/ctor-1.txt 2>/dev/null || echo "  (not present)"
echo "--- ctor-2.txt:"; cat build/amiga/OpenTTD-SDL2/ctor-2.txt 2>/dev/null || echo "  (not present)"
echo "--- run.log:";    cat build/amiga/OpenTTD-SDL2/run.log 2>/dev/null  || echo "  (not present)"
echo ""
echo "--- last 15 OTTD/AMIGA/DP/DBP markers:"
grep -E '^(AMIGA|OTTD|DBP|DP|GW|GL|DV): ' build/amiga/OpenTTD-SDL2/debug.log 2>/dev/null | tail -15 || echo "  (none)"
echo ""
echo "--- FS-UAE crash signatures:"
grep -E 'Gary timeout|Illegal instruction|Software Failure|Address Error' ~/Documents/FS-UAE/Cache/Logs/fs-uae.log.txt 2>/dev/null | tail -3 || echo "  (none)"
echo ""
kill $FSPID 2>/dev/null
sleep 2
pkill -f fs-uae 2>/dev/null || true
echo "FS-UAE killed."
