/*
 * test_usergroup.c -- Tests for amiport user/group, sysinfo, signal,
 *                     and file operation shim extensions.
 */

#include "test_framework.h"
#define AMIPORT_NO_PWD_MACROS
#define AMIPORT_NO_GRP_MACROS
#define AMIPORT_NO_UTSNAME_MACROS
#define AMIPORT_NO_SIGNAL_MACROS
#define AMIPORT_NO_LOCALE_MACROS
#define AMIPORT_NO_FILEOPS_MACROS
#include <amiport/pwd.h>
#include <amiport/grp.h>
#include <amiport/utsname.h>
#include <amiport/signal.h>
#include <amiport/unistd.h>
#include <amiport/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *verstag = "$VER: test_usergroup 1.0 (25.03.2026)";
long __stack = 32768;

/* ========== UID/GID ========== */

TEST(getuid_returns_zero)
{
    ASSERT_EQ(amiport_getuid(), 0);
}

TEST(geteuid_returns_zero)
{
    ASSERT_EQ(amiport_geteuid(), 0);
}

TEST(getgid_returns_zero)
{
    ASSERT_EQ(amiport_getgid(), 0);
}

TEST(getegid_returns_zero)
{
    ASSERT_EQ(amiport_getegid(), 0);
}

TEST(setuid_succeeds)
{
    ASSERT_EQ(amiport_setuid(1000), 0);
    /* Still returns 0 -- single user */
    ASSERT_EQ(amiport_getuid(), 0);
}

TEST(setgid_succeeds)
{
    ASSERT_EQ(amiport_setgid(1000), 0);
    ASSERT_EQ(amiport_getgid(), 0);
}

/* ========== Password Database ========== */

TEST(getpwuid_returns_amiga)
{
    struct amiport_passwd *pw;
    pw = amiport_getpwuid(0);
    ASSERT_NOT_NULL(pw);
    ASSERT_STR_EQ(pw->pw_name, "amiga");
    ASSERT_EQ(pw->pw_uid, 0);
    ASSERT_EQ(pw->pw_gid, 0);
    ASSERT_STR_EQ(pw->pw_dir, "SYS:");
    ASSERT_STR_EQ(pw->pw_shell, "C:Shell");
}

TEST(getpwuid_any_uid)
{
    struct amiport_passwd *pw;
    /* Any UID returns the same user on AmigaOS */
    pw = amiport_getpwuid(1000);
    ASSERT_NOT_NULL(pw);
    ASSERT_STR_EQ(pw->pw_name, "amiga");
}

TEST(getpwnam_returns_amiga)
{
    struct amiport_passwd *pw;
    pw = amiport_getpwnam("root");
    ASSERT_NOT_NULL(pw);
    ASSERT_STR_EQ(pw->pw_name, "amiga");
}

/* ========== Group Database ========== */

TEST(getgrgid_returns_amiga)
{
    struct amiport_group *gr;
    gr = amiport_getgrgid(0);
    ASSERT_NOT_NULL(gr);
    ASSERT_STR_EQ(gr->gr_name, "amiga");
    ASSERT_EQ(gr->gr_gid, 0);
}

TEST(getgrnam_returns_amiga)
{
    struct amiport_group *gr;
    gr = amiport_getgrnam("wheel");
    ASSERT_NOT_NULL(gr);
    ASSERT_STR_EQ(gr->gr_name, "amiga");
}

TEST(getgroups_returns_one)
{
    int groups[4];
    int n;
    n = amiport_getgroups(4, groups);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(groups[0], 0);
}

TEST(getgroups_zero_returns_count)
{
    int n;
    n = amiport_getgroups(0, NULL);
    ASSERT_EQ(n, 1);
}

TEST(getgrouplist_returns_one)
{
    int groups[4];
    int ngroups = 4;
    int ret;
    ret = amiport_getgrouplist("amiga", 0, groups, &ngroups);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(ngroups, 1);
    ASSERT_EQ(groups[0], 0);
}

/* ========== Login/TTY ========== */

TEST(getlogin_returns_amiga)
{
    char *name;
    name = amiport_getlogin();
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "amiga");
}

TEST(ttyname_returns_console)
{
    char *name;
    name = amiport_ttyname(0);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "CONSOLE:");
}

/* ========== OpenBSD Convenience ========== */

TEST(user_from_uid_returns_amiga)
{
    const char *name;
    name = amiport_user_from_uid(0, 0);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "amiga");
}

TEST(group_from_gid_returns_amiga)
{
    const char *name;
    name = amiport_group_from_gid(0, 0);
    ASSERT_NOT_NULL(name);
    ASSERT_STR_EQ(name, "amiga");
}

/* ========== System Info ========== */

TEST(gethostname_returns_something)
{
    char name[65];
    int ret;
    ret = amiport_gethostname(name, sizeof(name));
    ASSERT_EQ(ret, 0);
    /* On real AmigaOS: "amiga" or hostname from TCP/IP stack.
     * On vamos: GetVar may fail; fallback is "amiga". */
    ASSERT_STR_EQ(name, "amiga");
}

TEST(gethostname_null_fails)
{
    int ret;
    ret = amiport_gethostname(NULL, 64);
    ASSERT_EQ(ret, -1);
}

TEST(sysconf_pagesize)
{
    long val;
    val = amiport_sysconf(AMIPORT_SC_PAGESIZE);
    ASSERT_EQ(val, 4096);
}

TEST(sysconf_open_max)
{
    long val;
    val = amiport_sysconf(AMIPORT_SC_OPEN_MAX);
    ASSERT_EQ(val, 64);
}

TEST(sysconf_clk_tck)
{
    long val;
    val = amiport_sysconf(AMIPORT_SC_CLK_TCK);
    ASSERT_EQ(val, 50);
}

TEST(sysconf_invalid)
{
    long val;
    val = amiport_sysconf(9999);
    ASSERT_EQ(val, -1);
}

TEST(uname_fills_struct)
{
    struct amiport_utsname buf;
    int ret;
    ret = amiport_uname(&buf);
    ASSERT_EQ(ret, 0);
    ASSERT_STR_EQ(buf.sysname, "AmigaOS");
    ASSERT_STR_EQ(buf.release, "3.1");
    ASSERT_STR_EQ(buf.machine, "m68k");
}

TEST(getrusage_returns_zeroes)
{
    struct amiport_rusage usage;
    int ret;
    ret = amiport_getrusage(AMIPORT_RUSAGE_SELF, &usage);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(usage.ru_utime_sec, 0);
    ASSERT_EQ(usage.ru_maxrss, 0);
}

TEST(getrusage_null_fails)
{
    int ret;
    ret = amiport_getrusage(0, NULL);
    ASSERT_EQ(ret, -1);
}

/* ========== getprogname ========== */

TEST(getprogname_returns_something)
{
    /* __progname is set to "?" by default in argv_expand.c */
    ASSERT_NOT_NULL(__progname);
    ASSERT(strlen(__progname) > 0);
}

/* ========== Signal Extensions ========== */

TEST(sigemptyset_clears)
{
    amiport_sigset_t set = 0xFFFFFFFF;
    int ret;
    ret = amiport_sigemptyset(&set);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(set, 0);
}

TEST(sigaddset_sets_bit)
{
    amiport_sigset_t set = 0;
    amiport_sigemptyset(&set);
    amiport_sigaddset(&set, 2); /* SIGINT */
    ASSERT(set != 0);
}

TEST(sigaddset_invalid)
{
    amiport_sigset_t set = 0;
    int ret;
    ret = amiport_sigaddset(&set, 0);
    ASSERT_EQ(ret, -1);
    ret = amiport_sigaddset(&set, 32);
    ASSERT_EQ(ret, -1);
}

TEST(sigaction_stores_handler)
{
    struct amiport_sigaction act, oact;
    int ret;

    act.sa_handler = AMIPORT_SIG_IGN;
    act.sa_mask = 0;
    act.sa_flags = 0;
    ret = amiport_sigaction(AMIPORT_SIGINT, &act, NULL);
    ASSERT_EQ(ret, 0);

    /* Query old action */
    ret = amiport_sigaction(AMIPORT_SIGINT, NULL, &oact);
    ASSERT_EQ(ret, 0);
    ASSERT(oact.sa_handler == AMIPORT_SIG_IGN);

    /* Restore default */
    act.sa_handler = AMIPORT_SIG_DFL;
    amiport_sigaction(AMIPORT_SIGINT, &act, NULL);
}

TEST(sigaction_invalid_sig)
{
    struct amiport_sigaction act;
    int ret;
    act.sa_handler = AMIPORT_SIG_DFL;
    act.sa_mask = 0;
    act.sa_flags = 0;
    ret = amiport_sigaction(0, &act, NULL);
    ASSERT_EQ(ret, -1);
}

TEST(sigprocmask_noop)
{
    amiport_sigset_t set, oset;
    int ret;
    amiport_sigemptyset(&set);
    amiport_sigaddset(&set, AMIPORT_SIGINT);
    ret = amiport_sigprocmask(AMIPORT_SIG_BLOCK, &set, &oset);
    ASSERT_EQ(ret, 0);
}

/* ========== File Ops ========== */

TEST(fchmod_noop)
{
    int ret;
    ret = amiport_fchmod(1, 0644);
    ASSERT_EQ(ret, 0);
}

TEST(fchown_noop)
{
    int ret;
    ret = amiport_fchown(1, 0, 0);
    ASSERT_EQ(ret, 0);
}

TEST(lchown_noop)
{
    int ret;
    ret = amiport_lchown("T:nonexistent", 0, 0);
    ASSERT_EQ(ret, 0);
}

/* ========== errc ========== */

/* errc exits, so we can't test it directly without fork.
 * Just test that strtonum (already in err.h) still works. */
TEST(strtonum_basic)
{
    const char *errstr;
    long long val;
    val = amiport_strtonum("42", 0, 100, &errstr);
    ASSERT_EQ(val, 42);
    ASSERT_NULL(errstr);
}

TEST(strtonum_range_error)
{
    const char *errstr;
    long long val;
    val = amiport_strtonum("200", 0, 100, &errstr);
    ASSERT_EQ(val, 0);
    ASSERT_NOT_NULL(errstr);
}

/* ========== localeconv ========== */

TEST(localeconv_returns_c_defaults)
{
    struct amiport_lconv *lc;
    lc = amiport_localeconv();
    ASSERT_NOT_NULL(lc);
    ASSERT_STR_EQ(lc->decimal_point, ".");
    ASSERT_STR_EQ(lc->negative_sign, "-");
}

/* ========== timegm ========== */

TEST(timegm_epoch)
{
    struct tm t;
    long result;
    memset(&t, 0, sizeof(t));
    t.tm_year = 70;  /* 1970 */
    t.tm_mon = 0;    /* January */
    t.tm_mday = 1;
    result = amiport_timegm(&t);
    ASSERT_EQ(result, 0);
}

TEST(timegm_known_date)
{
    struct tm t;
    long result;
    memset(&t, 0, sizeof(t));
    t.tm_year = 100; /* 2000 */
    t.tm_mon = 0;    /* January */
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    result = amiport_timegm(&t);
    /* Jan 1, 2000 00:00:00 UTC = 946684800 */
    ASSERT_EQ(result, 946684800L);
}

/* ========== RUN ALL ========== */

int main(void)
{
    printf("test_usergroup: %s\n", verstag + 6);

    /* UID/GID */
    RUN_TEST(getuid_returns_zero);
    RUN_TEST(geteuid_returns_zero);
    RUN_TEST(getgid_returns_zero);
    RUN_TEST(getegid_returns_zero);
    RUN_TEST(setuid_succeeds);
    RUN_TEST(setgid_succeeds);

    /* Password database */
    RUN_TEST(getpwuid_returns_amiga);
    RUN_TEST(getpwuid_any_uid);
    RUN_TEST(getpwnam_returns_amiga);

    /* Group database */
    RUN_TEST(getgrgid_returns_amiga);
    RUN_TEST(getgrnam_returns_amiga);
    RUN_TEST(getgroups_returns_one);
    RUN_TEST(getgroups_zero_returns_count);
    RUN_TEST(getgrouplist_returns_one);

    /* Login/TTY */
    RUN_TEST(getlogin_returns_amiga);
    RUN_TEST(ttyname_returns_console);

    /* OpenBSD convenience */
    RUN_TEST(user_from_uid_returns_amiga);
    RUN_TEST(group_from_gid_returns_amiga);

    /* System info */
    RUN_TEST(gethostname_returns_something);
    RUN_TEST(gethostname_null_fails);
    RUN_TEST(sysconf_pagesize);
    RUN_TEST(sysconf_open_max);
    RUN_TEST(sysconf_clk_tck);
    RUN_TEST(sysconf_invalid);
    RUN_TEST(uname_fills_struct);
    RUN_TEST(getrusage_returns_zeroes);
    RUN_TEST(getrusage_null_fails);

    /* getprogname */
    RUN_TEST(getprogname_returns_something);

    /* Signal extensions */
    RUN_TEST(sigemptyset_clears);
    RUN_TEST(sigaddset_sets_bit);
    RUN_TEST(sigaddset_invalid);
    RUN_TEST(sigaction_stores_handler);
    RUN_TEST(sigaction_invalid_sig);
    RUN_TEST(sigprocmask_noop);

    /* File ops */
    RUN_TEST(fchmod_noop);
    RUN_TEST(fchown_noop);
    RUN_TEST(lchown_noop);

    /* err.h */
    RUN_TEST(strtonum_basic);
    RUN_TEST(strtonum_range_error);

    /* localeconv */
    RUN_TEST(localeconv_returns_c_defaults);

    /* timegm */
    RUN_TEST(timegm_epoch);
    RUN_TEST(timegm_known_date);

    return test_summary();
}
