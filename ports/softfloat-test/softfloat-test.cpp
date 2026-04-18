/*
 * lib/softfloat isolation test (PDR-015 OpenTTD debugging)
 *
 * Goal: validate that lib/softfloat (pure-integer IEEE 754 + Sun fdlibm)
 * is sound on FS-UAE BEFORE blaming OpenTTD-specific code for the
 * post-CheckMD5 Guru #80000003 alignment trap.
 *
 * If this binary runs cleanly to "ALL TESTS PASSED" with all expected
 * values, lib/softfloat is not the culprit and the OpenTTD crash is
 * elsewhere (most likely MD5 buffer alignment or a packed struct field).
 *
 * If this binary Gurus, the bug is in lib/softfloat itself and we fix
 * it once for every future C++ port.
 *
 * Build flags match openttd exactly: -m68020 -O1 -noixemul -std=c++17.
 * Link order: libsoftfloat.a BEFORE -lm so soft-float helpers win.
 */

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <string>
#include <sstream>
#include <cassert>

/* fmt 7.x (vendored in OpenTTD as v7) header-only mode -- bring in
 * OpenTTD's vendored copy so we test the EXACT same fmt that OpenTTD's
 * Debug() macro routes through. */
#define FMT_HEADER_ONLY
#define FMT_EXCEPTIONS 0
#include "fmt/format.h"

long __stack = 262144;

static int log_fd = -1;

/* Global ctor probe -- if C++ global constructors run before main(),
 * this object's ctor will fire FIRST and write to /T/ctor-probe.txt.
 * If main() runs but this file is empty or absent, libnix-noixemul
 * is NOT walking _init_array, so libstdc++'s locale singletons are
 * uninitialized. That would explain why ostringstream<<int Gurus:
 * use_facet<num_put<char>>(getloc()) dispatches through a poisoned
 * vtable -> privilege violation (#80000008). */
struct CtorProbe {
    CtorProbe() {
        FILE *f = fopen("WORK:softfloat-test/ctor-probe.txt", "w");
        if (f) {
            fputs("CTOR_PROBE: global ctor RAN before main\n", f);
            fclose(f);
        }
    }
};
static CtorProbe g_ctor_probe;

static void
dlog(const char *m)
{
    if (log_fd < 0) {
        log_fd = open("WORK:softfloat-test/softfloat-test.log",
                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    if (log_fd >= 0) {
        write(log_fd, m, strlen(m));
        write(log_fd, "\n", 1);
    }
    printf("%s\n", m);
}

static void
logf_(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    dlog(buf);
}

/* Compare with tolerance -- soft-float bit-exactness isn't required,
 * we just need values close to the mathematical truth. */
static int
near(double actual, double expected, double tol)
{
    double d = actual - expected;
    if (d < 0) d = -d;
    return d < tol;
}

int
main(int argc, char **argv)
{
    int failures = 0;

    dlog("SFT: main entered");

    /* ---- Phase 1: __divsf3 / __mulsf3 / __addsf3 / __subsf3 (single) ---- */
    dlog("SFT: phase 1 -- single-precision soft-float helpers");
    {
        volatile float a = 3.0f, b = 2.0f;
        volatile float r;

        r = a + b;
        logf_("SFT: 3.0f + 2.0f = %.6f (expect 5.0)", (double)r);
        if (!near((double)r, 5.0, 1e-5)) failures++;

        r = a - b;
        logf_("SFT: 3.0f - 2.0f = %.6f (expect 1.0)", (double)r);
        if (!near((double)r, 1.0, 1e-5)) failures++;

        r = a * b;
        logf_("SFT: 3.0f * 2.0f = %.6f (expect 6.0)", (double)r);
        if (!near((double)r, 6.0, 1e-5)) failures++;

        r = a / b;
        logf_("SFT: 3.0f / 2.0f = %.6f (expect 1.5)", (double)r);
        if (!near((double)r, 1.5, 1e-5)) failures++;
    }

    /* ---- Phase 2: __divdf3 / __muldf3 / __adddf3 / __subdf3 (double) ---- */
    dlog("SFT: phase 2 -- double-precision soft-float helpers");
    {
        volatile double a = 7.0, b = 2.5;
        volatile double r;

        r = a + b;
        logf_("SFT: 7.0 + 2.5 = %.10f (expect 9.5)", r);
        if (!near(r, 9.5, 1e-9)) failures++;

        r = a - b;
        logf_("SFT: 7.0 - 2.5 = %.10f (expect 4.5)", r);
        if (!near(r, 4.5, 1e-9)) failures++;

        r = a * b;
        logf_("SFT: 7.0 * 2.5 = %.10f (expect 17.5)", r);
        if (!near(r, 17.5, 1e-9)) failures++;

        r = a / b;
        logf_("SFT: 7.0 / 2.5 = %.10f (expect 2.8)", r);
        if (!near(r, 2.8, 1e-9)) failures++;
    }

    /* ---- Phase 3: int<->float conversion ---- */
    dlog("SFT: phase 3 -- int<->float conversion");
    {
        volatile int i = 42;
        volatile float f = (float)i;
        volatile int back = (int)f;
        logf_("SFT: int 42 -> float %.1f -> int %d (expect 42)", (double)f, back);
        if (back != 42) failures++;

        volatile int neg = -1234;
        volatile double d = (double)neg;
        volatile int back2 = (int)d;
        logf_("SFT: int -1234 -> double %.1f -> int %d (expect -1234)", d, back2);
        if (back2 != -1234) failures++;
    }

    /* ---- Phase 4: software libm -- sqrt / fabs / floor ---- */
    dlog("SFT: phase 4 -- libm: sqrt / fabs / floor");
    {
        volatile double r;

        r = sqrt(2.0);
        logf_("SFT: sqrt(2.0) = %.10f (expect ~1.41421356)", r);
        if (!near(r, 1.41421356237, 1e-6)) failures++;

        r = sqrt(64.0);
        logf_("SFT: sqrt(64.0) = %.10f (expect 8.0)", r);
        if (!near(r, 8.0, 1e-9)) failures++;

        r = fabs(-3.5);
        logf_("SFT: fabs(-3.5) = %.10f (expect 3.5)", r);
        if (!near(r, 3.5, 1e-9)) failures++;

        r = floor(3.7);
        logf_("SFT: floor(3.7) = %.10f (expect 3.0)", r);
        if (!near(r, 3.0, 1e-9)) failures++;
    }

    /* ---- Phase 5: software libm -- sin / cos / tan ---- */
    dlog("SFT: phase 5 -- libm: sin / cos / tan");
    {
        volatile double pi = 3.14159265358979323846;
        volatile double r;

        r = sin(0.0);
        logf_("SFT: sin(0) = %.10f (expect 0)", r);
        if (!near(r, 0.0, 1e-9)) failures++;

        r = sin(pi / 2.0);
        logf_("SFT: sin(pi/2) = %.10f (expect 1)", r);
        if (!near(r, 1.0, 1e-6)) failures++;

        r = cos(0.0);
        logf_("SFT: cos(0) = %.10f (expect 1)", r);
        if (!near(r, 1.0, 1e-9)) failures++;

        r = cos(pi);
        logf_("SFT: cos(pi) = %.10f (expect -1)", r);
        if (!near(r, -1.0, 1e-6)) failures++;

        r = tan(pi / 4.0);
        logf_("SFT: tan(pi/4) = %.10f (expect 1)", r);
        if (!near(r, 1.0, 1e-6)) failures++;
    }

    /* ---- Phase 6: software libm -- exp / log / pow ---- */
    dlog("SFT: phase 6 -- libm: exp / log / pow");
    {
        volatile double r;

        r = exp(0.0);
        logf_("SFT: exp(0) = %.10f (expect 1)", r);
        if (!near(r, 1.0, 1e-9)) failures++;

        r = exp(1.0);
        logf_("SFT: exp(1) = %.10f (expect e ~ 2.71828)", r);
        if (!near(r, 2.71828182846, 1e-5)) failures++;

        r = log(1.0);
        logf_("SFT: log(1) = %.10f (expect 0)", r);
        if (!near(r, 0.0, 1e-9)) failures++;

        r = log(2.71828182846);
        logf_("SFT: log(e) = %.10f (expect 1)", r);
        if (!near(r, 1.0, 1e-5)) failures++;

        r = pow(2.0, 10.0);
        logf_("SFT: pow(2,10) = %.10f (expect 1024)", r);
        if (!near(r, 1024.0, 1e-6)) failures++;

        r = pow(3.0, 0.5);
        logf_("SFT: pow(3,0.5) = %.10f (expect ~1.732)", r);
        if (!near(r, 1.73205080757, 1e-5)) failures++;
    }

    /* ---- Phase 7: software libm -- atan / atan2 ---- */
    dlog("SFT: phase 7 -- libm: atan / atan2");
    {
        volatile double r;
        volatile double pi = 3.14159265358979323846;

        r = atan(1.0);
        logf_("SFT: atan(1) = %.10f (expect pi/4 ~ 0.7854)", r);
        if (!near(r, pi / 4.0, 1e-6)) failures++;

        r = atan2(1.0, 1.0);
        logf_("SFT: atan2(1,1) = %.10f (expect pi/4)", r);
        if (!near(r, pi / 4.0, 1e-6)) failures++;

        r = atan2(0.0, -1.0);
        logf_("SFT: atan2(0,-1) = %.10f (expect pi)", r);
        if (!near(r, pi, 1e-6)) failures++;
    }

    /* ---- Phase 8: float wrappers (sinf, cosf, sqrtf) ---- */
    dlog("SFT: phase 8 -- libm: sinf / cosf / sqrtf");
    {
        volatile float r;

        r = sqrtf(16.0f);
        logf_("SFT: sqrtf(16) = %.6f (expect 4)", (double)r);
        if (!near((double)r, 4.0, 1e-5)) failures++;

        r = sinf(0.0f);
        logf_("SFT: sinf(0) = %.6f (expect 0)", (double)r);
        if (!near((double)r, 0.0, 1e-5)) failures++;

        r = cosf(0.0f);
        logf_("SFT: cosf(0) = %.6f (expect 1)", (double)r);
        if (!near((double)r, 1.0, 1e-5)) failures++;
    }

    /* ---- Phase 9: printf %f / %g (stdio float formatting) ---- */
    dlog("SFT: phase 9 -- printf %f / %g formatting");
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "SFT: printf %%f: 3.14 -> '%f'", 3.14);
        dlog(buf);
        snprintf(buf, sizeof(buf), "SFT: printf %%.3f: 3.14159 -> '%.3f'", 3.14159);
        dlog(buf);
        snprintf(buf, sizeof(buf), "SFT: printf %%g: 3.14 -> '%g'", 3.14);
        dlog(buf);
        snprintf(buf, sizeof(buf), "SFT: printf %%g: 1234567.0 -> '%g'", 1234567.0);
        dlog(buf);
    }

    /* ---- Phase 10: std::ostringstream -- bisect to find which operator<< crashes
     *
     * Phases 1-9 passed cleanly on FS-UAE (run 1, 2026-04-16). Run 1's
     * combined ostringstream test crashed mid-phase with no further output.
     * This bisection isolates exactly which operator<< overload is the
     * trigger so we know whether to fix lib/softfloat (if double is
     * involved) or fix libstdc++ wiring (if int/string is involved).
     */
    dlog("SFT: phase 10a -- ostringstream construct + literal string");
    {
        std::ostringstream oss;
        oss << "hello";
        std::string s = oss.str();
        logf_("SFT: 10a oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b1 -- ostringstream.write() raw bytes (no facet)");
    {
        std::ostringstream oss;
        oss.write("42", 2);
        std::string s = oss.str();
        logf_("SFT: 10b1 oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b2 -- ostringstream.put() one byte at a time");
    {
        std::ostringstream oss;
        oss.put('4');
        oss.put('2');
        std::string s = oss.str();
        logf_("SFT: 10b2 oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b3 -- query oss.flags() (cheap state read)");
    {
        std::ostringstream oss;
        std::ios_base::fmtflags f = oss.flags();
        logf_("SFT: 10b3 flags=0x%lx (just reading, no facet dispatch)", (unsigned long)f);
    }

    dlog("SFT: phase 10b4 -- std::locale::classic() reachable?");
    {
        const std::locale &loc = std::locale::classic();
        /* Just touching it -- if global ctors didn't run, this Gurus right here. */
        logf_("SFT: 10b4 locale.name() about to be queried...");
        std::string n = loc.name();
        logf_("SFT: 10b4 locale.name() returned size=%lu", (unsigned long)n.size());
    }

    /* Phases 10b5c-10b5e are GATED OUT by default -- they reproduce the
     * already-documented bebbo-gcc 13.3 std::ostream operator<<(short)
     * Guru #80000008 (see known-pitfalls.md). Each run that includes
     * them halts the binary mid-test. Set ENABLE_OSTREAM_INT_REPRO=1
     * at compile time to re-run the bisect (e.g. when validating a
     * compiler upgrade). */
#if ENABLE_OSTREAM_INT_REPRO
    dlog("SFT: phase 10b5a -- oss << (long)42");
    {
        std::ostringstream oss;
        oss << (long)42;
        std::string s = oss.str();
        logf_("SFT: 10b5a content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b5b -- oss << (unsigned long)42");
    {
        std::ostringstream oss;
        oss << (unsigned long)42;
        std::string s = oss.str();
        logf_("SFT: 10b5b content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b5c -- oss << (short)42");
    {
        std::ostringstream oss;
        oss << (short)42;
        std::string s = oss.str();
        logf_("SFT: 10b5c content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b5d -- oss << (int)42 (the original suspect)");
    {
        std::ostringstream oss;
        oss << (int)42;
        std::string s = oss.str();
        logf_("SFT: 10b5d content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10b5e -- oss << (void*)0x1234 (pointer overload)");
    {
        std::ostringstream oss;
        oss << (void *)(unsigned long)0x1234UL;
        std::string s = oss.str();
        logf_("SFT: 10b5e content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10c -- ostringstream << float");
    {
        std::ostringstream oss;
        oss << 2.71828f;
        std::string s = oss.str();
        logf_("SFT: 10c oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10d -- ostringstream << double");
    {
        std::ostringstream oss;
        oss << 3.14159;
        std::string s = oss.str();
        logf_("SFT: 10d oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 10e -- ostringstream chained mixed");
    {
        std::ostringstream oss;
        oss << "double=" << 3.14159 << " float=" << 2.71828f << " int=" << 42;
        std::string s = oss.str();
        logf_("SFT: 10e oss content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    /* ---- Phase 11: fmt::format -- the OpenTTD Debug() critical path ----
     *
     * OpenTTD's Debug(name, level, fmtstr, ...) macro expands to
     * DebugPrint(#name, fmt::format(FMT_STRING(fmtstr), ...)). All 604
     * Debug() call sites in OpenTTD route through fmt::format, NOT
     * through std::ostream<<. fmt has its own hand-rolled int-formatting
     * (fmt::detail::format_int) that bypasses libstdc++'s num_put facet
     * entirely.
     *
     * If 11b crashes -> the bebbo-gcc 13.3 ostream<<int defect ALSO
     * affects fmt's internals -> all OpenTTD Debug() calls broken ->
     * must replace Debug() with snprintf-based logger.
     *
     * If 11b works -> fmt is independent of the ostream defect ->
     * OpenTTD's post-CheckMD5 #80000003 alignment trap is genuinely
     * unrelated to libstdc++ -> deploy Enforcer to find the alignment
     * site directly.
     */
#endif /* ENABLE_OSTREAM_INT_REPRO */

    dlog("SFT: phase 11a -- fmt::format string only");
    {
        std::string s = fmt::format("hello {}", "world");
        logf_("SFT: 11a content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 11b -- fmt::format with int (the OpenTTD Debug() critical path)");
    {
        std::string s = fmt::format("answer={}", 42);
        logf_("SFT: 11b content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 11c -- fmt::format with FMT_STRING (exact Debug() macro pattern)");
    {
        std::string s = fmt::format(FMT_STRING("answer={}"), 42);
        logf_("SFT: 11c content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    dlog("SFT: phase 11d -- fmt::format with float (Debug() emits these via {:.1f})");
    {
        std::string s = fmt::format("ratio={:.1f}", 3.14);
        logf_("SFT: 11d content: '%s' size=%lu", s.c_str(), (unsigned long)s.size());
    }

    /* ---- Final verdict ---- */
    if (failures == 0) {
        dlog("SFT: ALL TESTS PASSED");
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "SFT: FAILURES = %d", failures);
        dlog(buf);
    }

    if (log_fd >= 0) {
        close(log_fd);
    }

    return failures == 0 ? 0 : 10;
}
