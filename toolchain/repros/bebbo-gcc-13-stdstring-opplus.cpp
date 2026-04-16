/*
 * Minimal reproduction: bebbo-gcc 13.3 std::string return-by-value bug
 *
 * Build:  m68k-amigaos-g++ -std=c++17 -m68040 -m68881 -O0 -noixemul \
 *           -D__libnix__ -o repro repro.cpp -lstdc++
 * Run on FS-UAE (A3000, 68030+, 64MB Z3 RAM).
 *
 * Expected if working: prints "OK: result=hello world" and exits 0.
 * Actual: throws std::out_of_range with garbage __pos / size values,
 *         OR Guru Meditation #80000004 (Illegal Instruction).
 */

#include <stdio.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>

long __stack = 65536;

static int debug_fd = -1;
static void dlog(const char *m) {
    if (debug_fd < 0) {
        debug_fd = open("WORK:OpenTTD/repro.log", O_WRONLY|O_CREAT|O_TRUNC, 0644);
    }
    if (debug_fd >= 0) {
        write(debug_fd, m, __builtin_strlen(m));
        write(debug_fd, "\n", 1);
    }
    printf("%s\n", m);
}

/* The problematic pattern: function returns std::string by value. */
static std::string make_string()
{
    std::string r("hello");
    r += " world";
    return r;
}

int main(int argc, char **argv)
{
    dlog("REPRO: main entered");

    dlog("REPRO: test 1: std::string ctor + append (no return-by-value)");
    {
        std::string s("hello");
        s += " world";
        char buf[100];
        snprintf(buf, sizeof(buf), "REPRO: t1 size=%lu val=%s", (unsigned long)s.size(), s.c_str());
        dlog(buf);
    }

    dlog("REPRO: test 2: std::string returned by value from function");
    {
        std::string s = make_string();
        char buf[100];
        snprintf(buf, sizeof(buf), "REPRO: t2 size=%lu val=%s", (unsigned long)s.size(), s.c_str());
        dlog(buf);
    }

    dlog("REPRO: test 3: std::string operator+ small (fits SBO)");
    {
        std::string a("hello");
        std::string b = a + " world";
        char buf[100];
        snprintf(buf, sizeof(buf), "REPRO: t3 size=%lu val=%s", (unsigned long)b.size(), b.c_str());
        dlog(buf);
    }

    dlog("REPRO: test 4: std::string operator+ medium (>15 chars, heap)");
    {
        std::string a("WORK:OpenTTD/");
        std::string b = a + "PROGDIR:data";
        char buf[100];
        snprintf(buf, sizeof(buf), "REPRO: t4 size=%lu val=%s", (unsigned long)b.size(), b.c_str());
        dlog(buf);
    }

    dlog("REPRO: test 5: std::string operator+ at openttd's exact case");
    {
        std::string a("WORK:OpenTTD/");
        const char *PD = "PROGDIR:data";
        std::string b = a + PD;
        char buf[100];
        snprintf(buf, sizeof(buf), "REPRO: t5 size=%lu val=%s", (unsigned long)b.size(), b.c_str());
        dlog(buf);
    }

    dlog("REPRO: ALL PASSED");
    return 0;
}
