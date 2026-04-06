/*
 * amiport.c -- Package manager for classic AmigaOS 3.x
 *
 * The first package manager for 68k AmigaOS. Downloads, verifies,
 * and installs packages from the amiport API.
 *
 * Commands: list, search, info, install, upgrade, remove,
 *           installed, doctor, help
 *
 * amiport: original code (not a port)
 */

static const char *verstag = "$VER: amiport 1.0 (05.04.2026)";
long __stack = 65536;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amiport/stdlib.h>
#include <amiport-net/http.h>
#include <amiport-net/socket.h>
#include <amiport-net/netdb.h>
#include <amiport-net/netinet/in.h>

#include "json.h"
#include "sha256.h"
#include "db.h"
#include "config.h"

#ifdef __AMIGA__
#include <proto/exec.h>
#include <proto/dos.h>
#include <amiport/signal.h>
#endif

/* --- Globals --- */

static struct amiport_config cfg;
static struct amiport_package manifest[AMIPORT_MAX_PACKAGES];
static int manifest_count = 0;
static int manifest_loaded = 0;
static int use_color = 0;

#define CACHE_PATH "S:amiport-cache.json"

/* --- ANSI color helpers --- */

#define COL_AMBER   "\033[33m"
#define COL_GREEN   "\033[32m"
#define COL_RED     "\033[31m"
#define COL_BOLD    "\033[1m"
#define COL_RESET   "\033[0m"

static void color_on(const char *code)
{
    if (use_color) fputs(code, stdout);
}

static void color_off(void)
{
    if (use_color) fputs(COL_RESET, stdout);
}

/* --- Manifest loading --- */

static int fetch_manifest(void)
{
    char url[256];
    int http_status;
    int rc;

    snprintf(url, sizeof(url), "%s/api/v1/packages.php", cfg.server);

    rc = amiport_http_get(url, CACHE_PATH, &http_status, NULL);
    if (rc != 0) {
        fprintf(stderr, "amiport: cannot fetch package list from %s\n",
                cfg.server);
        return -1;
    }
    return 0;
}

static int load_manifest(void)
{
    FILE *fp;

    if (manifest_loaded) return manifest_count;

    /* Try cache first */
    fp = fopen(CACHE_PATH, "r");
    if (!fp) {
        /* No cache, fetch from server */
        if (fetch_manifest() != 0) return -1;
    } else {
        fclose(fp);
    }

    manifest_count = amiport_parse_manifest(CACHE_PATH, manifest,
                                            AMIPORT_MAX_PACKAGES);
    if (manifest_count < 0) {
        /* Corrupt cache, re-fetch */
        remove(CACHE_PATH);
        if (fetch_manifest() != 0) return -1;
        manifest_count = amiport_parse_manifest(CACHE_PATH, manifest,
                                                AMIPORT_MAX_PACKAGES);
    }

    if (manifest_count >= AMIPORT_MAX_PACKAGES) {
        fprintf(stderr, "amiport: warning: package list truncated "
                "at %d entries\n", AMIPORT_MAX_PACKAGES);
    }

    if (manifest_count > 0) manifest_loaded = 1;
    return manifest_count;
}

static struct amiport_package *find_package(const char *name)
{
    int i;
    for (i = 0; i < manifest_count; i++) {
        if (strcmp(manifest[i].name, name) == 0) return &manifest[i];
    }
    return NULL;
}

/* --- Progress callback --- */

static void download_progress(long received, long total)
{
    if (total > 0) {
        fprintf(stderr, "\r  %ldKB/%ldKB",
                received / 1024, total / 1024);
    } else {
        fprintf(stderr, "\r  %ldKB", received / 1024);
    }
    (void)fflush(stderr);
}

/* --- Install helper (shared by install + upgrade) --- */

static int do_install(struct amiport_package *pkg)
{
    char url[512];
    char tmppath[128];
    char destpath[128];
    char hash[65];
    int http_status;
    int rc;
#ifdef __AMIGA__
    char lha_cmd[256];
    long sys_rc;
#endif

    /* Construct full download URL */
    snprintf(url, sizeof(url), "%s%s?format=machine",
             cfg.server, pkg->download);

    snprintf(tmppath, sizeof(tmppath), "T:%s-%s.lha",
             pkg->name, pkg->version);

    /* Download */
    fprintf(stderr, "Downloading %s %s...\n", pkg->name, pkg->version);
    rc = amiport_http_get(url, tmppath, &http_status, download_progress);
    fprintf(stderr, "\n");

    if (rc != 0) {
        fprintf(stderr, "amiport: download failed\n");
        remove(tmppath);
        return -1;
    }

    /* SHA256 verification */
    if (pkg->machine_sha256[0]) {
        fprintf(stderr, "Verifying... ");
        sha256_file(tmppath, hash);
        if (strcmp(hash, pkg->machine_sha256) != 0) {
            fprintf(stderr, "FAILED\n");
            fprintf(stderr, "amiport: download corrupted "
                    "(expected %.8s..., got %.8s...)\n",
                    pkg->machine_sha256, hash);
            remove(tmppath);
            return -1;
        }
        fprintf(stderr, "OK\n");
    } else if (pkg->sha256[0]) {
        /* Fallback to regular sha256 if no machine_sha256 */
        fprintf(stderr, "Verifying... ");
        sha256_file(tmppath, hash);
        if (strcmp(hash, pkg->sha256) != 0) {
            fprintf(stderr, "FAILED\n");
            fprintf(stderr, "amiport: download corrupted "
                    "(expected %.8s..., got %.8s...)\n",
                    pkg->sha256, hash);
            remove(tmppath);
            return -1;
        }
        fprintf(stderr, "OK\n");
    } else {
        fprintf(stderr, "Warning: no checksum available, "
                "skipping verification\n");
    }

#ifdef __AMIGA__
    /* Extract via lha */
    fprintf(stderr, "Extracting... ");
    snprintf(lha_cmd, sizeof(lha_cmd),
             "C:lha -q x \"%s\" SYS:", tmppath);
    sys_rc = SystemTags((CONST_STRPTR)lha_cmd, TAG_DONE);
    if (sys_rc != 0) {
        fprintf(stderr, "FAILED\n");
        fprintf(stderr, "amiport: extraction failed (lha rc=%ld)\n", sys_rc);
        remove(tmppath);
        return -1;
    }
    fprintf(stderr, "OK\n");

    /* Construct install path and verify binary exists */
    snprintf(destpath, sizeof(destpath), "%s%s",
             cfg.installpath, pkg->name);
    {
        BPTR lock = Lock((CONST_STRPTR)destpath, ACCESS_READ);
        if (!lock) {
            fprintf(stderr, "amiport: binary not found at %s after extraction\n",
                    destpath);
            remove(tmppath);
            return -1;
        }
        UnLock(lock);
    }
#else
    /* Non-Amiga: just pretend it worked for testing */
    snprintf(destpath, sizeof(destpath), "%s%s",
             cfg.installpath, pkg->name);
#endif

    /* Update DB */
    if (amiport_db_save(pkg->name, pkg->version, destpath) != 0) {
        fprintf(stderr, "amiport: warning: could not update "
                "package database\n");
    }

    /* Cleanup temp file */
    remove(tmppath);

    /* Success message */
    color_on(COL_GREEN);
    printf("Installed %s %s to %s\n", pkg->name, pkg->version, destpath);
    color_off();

    return 0;
}

/* --- Commands --- */

static void cmd_help(void)
{
    printf("amiport - Package manager for AmigaOS 3.x\n\n");
    printf("Usage: amiport <command> [arguments]\n\n");
    printf("Commands:\n");
    printf("  list                 Show all available packages\n");
    printf("  search <term>        Search packages by name/description\n");
    printf("  info <name>          Show package details\n");
    printf("  install <name>       Download and install a package\n");
    printf("  upgrade [name]       Upgrade installed packages\n");
    printf("  remove <name>        Uninstall a package\n");
    printf("  installed            Show installed packages\n");
    printf("  doctor               Check network connectivity\n");
    printf("  help                 Show this help\n");
    printf("\nConfig: S:amiport.conf  Database: S:amiport.db\n");
    printf("Server: %s\n", cfg.server);
}

static int cmd_list(void)
{
    int i;

    if (load_manifest() < 0) return 10;

    printf("%-16s %-10s %-40s %s\n",
           "Name", "Version", "Description", "Status");
    printf("%-16s %-10s %-40s %s\n",
           "----", "-------", "-----------", "------");

    for (i = 0; i < manifest_count; i++) {
        const struct amiport_installed *inst;
        const char *tag = "";

        inst = amiport_db_find(manifest[i].name);
        if (inst) {
            if (strcmp(inst->version, manifest[i].version) != 0) {
                tag = "[update]";
            } else {
                tag = "[installed]";
            }
        }

        color_on(COL_AMBER);
        printf("%-16s", manifest[i].name);
        color_off();
        printf(" %-10s %-40s ", manifest[i].version,
               manifest[i].description);
        if (tag[0] == '[' && tag[1] == 'u') {
            color_on(COL_BOLD);
            color_on(COL_AMBER);
        } else if (tag[0]) {
            color_on(COL_GREEN);
        }
        printf("%s", tag);
        color_off();
        printf("\n");
    }

    printf("\n%d packages available\n", manifest_count);
    return 0;
}

static int cmd_search(const char *term)
{
    int i;
    int found = 0;

    if (!term || !*term) {
        fprintf(stderr, "amiport: search requires a term\n");
        return 10;
    }

    if (load_manifest() < 0) return 10;

    /* perf: lowercase search term once before the loop */
    {
        char term_lower[64];
        int j;
        for (j = 0; term[j] && j < 63; j++)
            term_lower[j] = (term[j] >= 'A' && term[j] <= 'Z')
                ? term[j] + 32 : term[j];
        term_lower[j] = '\0';

    for (i = 0; i < manifest_count; i++) {
        /* Case-insensitive substring match on name + description */
        char name_lower[32];
        char desc_lower[80];
        int match = 0;

        for (j = 0; manifest[i].name[j] && j < 31; j++)
            name_lower[j] = (manifest[i].name[j] >= 'A' &&
                              manifest[i].name[j] <= 'Z')
                ? manifest[i].name[j] + 32 : manifest[i].name[j];
        name_lower[j] = '\0';

        for (j = 0; manifest[i].description[j] && j < 79; j++)
            desc_lower[j] = (manifest[i].description[j] >= 'A' &&
                              manifest[i].description[j] <= 'Z')
                ? manifest[i].description[j] + 32 : manifest[i].description[j];
        desc_lower[j] = '\0';

        if (strstr(name_lower, term_lower) ||
            strstr(desc_lower, term_lower)) {
            match = 1;
        }

        if (match) {
            const struct amiport_installed *inst;
            const char *tag = "";

            inst = amiport_db_find(manifest[i].name);
            if (inst) {
                if (strcmp(inst->version, manifest[i].version) != 0)
                    tag = "[update]";
                else
                    tag = "[installed]";
            }

            color_on(COL_AMBER);
            printf("%-16s", manifest[i].name);
            color_off();
            printf(" %-10s %-40s %s\n", manifest[i].version,
                   manifest[i].description, tag);
            found++;
        }
    }

    } /* end term_lower scope */

    if (found == 0) {
        printf("No packages match '%s'.\n", term);
    } else {
        printf("\n%d packages found\n", found);
    }
    return 0;
}

static int cmd_info(const char *name)
{
    struct amiport_package pkg;
    char url[256];
    char tmppath[] = "T:amiport-info.json";
    int http_status;
    int rc;
    int i;

    if (!name || !*name) {
        fprintf(stderr, "amiport: info requires a package name\n");
        return 10;
    }

    /* Try to fetch from API */
    snprintf(url, sizeof(url), "%s/api/v1/packages.php?name=%s",
             cfg.server, name);

    rc = amiport_http_get(url, tmppath, &http_status, NULL);
    if (rc != 0) {
        /* Fallback to cached manifest */
        struct amiport_package *cached;
        if (load_manifest() < 0) {
            fprintf(stderr, "amiport: cannot reach server or cache\n");
            return 10;
        }
        cached = find_package(name);
        if (!cached) {
            fprintf(stderr, "amiport: package '%s' not found\n", name);
            return 10;
        }
        printf("(cached data - run 'amiport list' to refresh)\n\n");
        color_on(COL_AMBER);
        printf("  %-16s %s\n", "Name:", cached->name);
        color_off();
        printf("  %-16s %s\n", "Version:", cached->version);
        printf("  %-16s %s\n", "Description:", cached->description);
        printf("  %-16s %s\n", "Category:", cached->category);
        if (cached->source[0])
            printf("  %-16s %s\n", "Source:", cached->source);
        if (cached->license[0])
            printf("  %-16s %s\n", "License:", cached->license);
        printf("  %-16s %ld bytes\n", "Size:", cached->size);
        return 0;
    }

    /* Parse with stream-printing for rich fields */
    rc = amiport_parse_package_info(tmppath, &pkg, stdout);
    remove(tmppath);

    if (rc != 0) {
        fprintf(stderr, "amiport: package '%s' not found\n", name);
        return 10;
    }

    /* Print structured fields */
    color_on(COL_AMBER);
    printf("  %-16s %s\n", "Name:", pkg.name);
    color_off();
    printf("  %-16s %s\n", "Version:", pkg.version);
    printf("  %-16s %s\n", "Description:", pkg.description);
    printf("  %-16s %s\n", "Category:", pkg.category);
    if (pkg.source[0])
        printf("  %-16s %s\n", "Source:", pkg.source);
    if (pkg.license[0])
        printf("  %-16s %s\n", "License:", pkg.license);
    printf("  %-16s %ld bytes\n", "Size:", pkg.size);
    if (pkg.sha256[0])
        printf("  %-16s %.16s...\n", "SHA256:", pkg.sha256);
    if (pkg.num_requires > 0) {
        printf("  %-16s ", "Requires:");
        for (i = 0; i < pkg.num_requires; i++) {
            if (i > 0) printf(", ");
            printf("%s", pkg.requires[i]);
        }
        printf("\n");
    }

    /* Check installed status */
    {
        const struct amiport_installed *inst = amiport_db_find(pkg.name);
        if (inst) {
            if (strcmp(inst->version, pkg.version) != 0) {
                color_on(COL_AMBER);
                printf("  %-16s %s (update available: %s)\n",
                       "Installed:", inst->version, pkg.version);
                color_off();
            } else {
                color_on(COL_GREEN);
                printf("  %-16s %s\n", "Installed:", inst->version);
                color_off();
            }
        }
    }

    return 0;
}

static int cmd_install(const char *name)
{
    struct amiport_package *pkg;
    const struct amiport_installed *inst;

    if (!name || !*name) {
        fprintf(stderr, "amiport: install requires a package name\n");
        return 10;
    }

    if (load_manifest() < 0) return 10;

    pkg = find_package(name);
    if (!pkg) {
        fprintf(stderr, "amiport: package '%s' not found. "
                "Run 'amiport list' to refresh.\n", name);
        return 10;
    }

    /* Check if already installed */
    inst = amiport_db_find(name);
    if (inst && strcmp(inst->version, pkg->version) == 0) {
        printf("%s %s is already installed.\n", name, inst->version);
        return 5; /* RETURN_WARN */
    }

#ifdef __AMIGA__
    /* Check lha exists */
    {
        BPTR lock = Lock((CONST_STRPTR)"C:lha", ACCESS_READ);
        if (!lock) {
            fprintf(stderr, "amiport: C:lha not found - "
                    "install lha first\n");
            return 10;
        }
        UnLock(lock);
    }

    /* Check requires (warn only) */
    {
        int i;
        for (i = 0; i < pkg->num_requires; i++) {
            struct Library *lib = OpenLibrary(
                (CONST_STRPTR)pkg->requires[i], 0);
            if (lib) {
                CloseLibrary(lib);
            } else {
                fprintf(stderr, "amiport: warning: %s not found "
                        "(may be required)\n", pkg->requires[i]);
            }
        }
    }
#endif

    return do_install(pkg);
}

static int cmd_upgrade(const char *name)
{
    struct amiport_installed entries[AMIPORT_MAX_INSTALLED];
    int count;
    int i;
    int outdated = 0;

    if (load_manifest() < 0) return 10;

    /* Single package upgrade */
    if (name && *name) {
        const struct amiport_installed *inst;
        struct amiport_package *pkg;

        /* Block self-update */
        if (strcmp(name, "amiport") == 0) {
            fprintf(stderr, "amiport: self-update not supported in v1. "
                    "Download manually from %s\n", cfg.server);
            return 10;
        }

        inst = amiport_db_find(name);
        if (!inst) {
            fprintf(stderr, "amiport: '%s' is not installed\n", name);
            return 10;
        }

        pkg = find_package(name);
        if (!pkg) {
            fprintf(stderr, "amiport: '%s' not in package list\n", name);
            return 10;
        }

        if (strcmp(inst->version, pkg->version) == 0) {
            printf("%s %s is already the latest version.\n",
                   name, inst->version);
            return 0;
        }

        printf("Upgrading %s %s -> %s\n",
               name, inst->version, pkg->version);
        return do_install(pkg);
    }

    /* Batch upgrade: check all installed */
    count = amiport_db_load(entries, AMIPORT_MAX_INSTALLED);
    if (count <= 0) {
        printf("No packages installed.\n");
        return 0;
    }

    /* List outdated */
    for (i = 0; i < count; i++) {
        struct amiport_package *pkg = find_package(entries[i].name);
        if (pkg && strcmp(entries[i].version, pkg->version) != 0) {
            if (strcmp(entries[i].name, "amiport") == 0) continue;
            if (outdated == 0) {
                printf("Outdated packages:\n");
            }
            printf("  %-16s %s -> %s\n",
                   entries[i].name, entries[i].version, pkg->version);
            outdated++;
        }
    }

    if (outdated == 0) {
        printf("All packages up to date.\n");
        return 0;
    }

    /* Prompt for confirmation */
    {
        char answer[8];
        printf("\nUpgrade %d package%s? (y/n) ",
               outdated, outdated > 1 ? "s" : "");
        (void)fflush(stdout);
        if (!fgets(answer, sizeof(answer), stdin) ||
            (answer[0] != 'y' && answer[0] != 'Y')) {
            printf("Cancelled.\n");
            return 0;
        }
    }

    /* Do the upgrades */
    for (i = 0; i < count; i++) {
        struct amiport_package *pkg = find_package(entries[i].name);
        if (pkg && strcmp(entries[i].version, pkg->version) != 0) {
            if (strcmp(entries[i].name, "amiport") == 0) continue;
            printf("\nUpgrading %s %s -> %s\n",
                   entries[i].name, entries[i].version, pkg->version);
            if (do_install(pkg) != 0) {
                fprintf(stderr, "amiport: upgrade of %s failed, "
                        "stopping\n", entries[i].name);
                return 10;
            }
        }
    }

    printf("\n%d package%s upgraded.\n",
           outdated, outdated > 1 ? "s" : "");
    return 0;
}

static int cmd_remove(const char *name)
{
    const struct amiport_installed *inst;
    int prefix_ok = 0;

    if (!name || !*name) {
        fprintf(stderr, "amiport: remove requires a package name\n");
        return 10;
    }

    inst = amiport_db_find(name);
    if (!inst) {
        fprintf(stderr, "amiport: '%s' is not installed\n", name);
        return 10;
    }

    /* Path prefix safety check */
    {
        int pfxlen = (int)strlen(cfg.installpath);
        if (strncmp(inst->path, cfg.installpath, pfxlen) == 0) {
            prefix_ok = 1;
        }
    }

    if (!prefix_ok) {
        fprintf(stderr, "amiport: refusing to delete %s "
                "(path outside install directory %s)\n",
                inst->path, cfg.installpath);
        return 10;
    }

#ifdef __AMIGA__
    /* Delete the binary */
    if (!DeleteFile((CONST_STRPTR)inst->path)) {
        /* File might already be gone */
        fprintf(stderr, "amiport: warning: file already deleted: %s\n",
                inst->path);
    }
#else
    remove(inst->path);
#endif

    /* Remove from DB */
    if (amiport_db_remove(name) != 0) {
        fprintf(stderr, "amiport: warning: could not update database\n");
    }

    color_on(COL_GREEN);
    printf("Removed %s\n", name);
    color_off();
    return 0;
}

static int cmd_installed(void)
{
    struct amiport_installed entries[AMIPORT_MAX_INSTALLED];
    int count;
    int i;

    count = amiport_db_load(entries, AMIPORT_MAX_INSTALLED);
    if (count <= 0) {
        printf("No packages installed.\n");
        return 0;
    }

    printf("%-16s %-10s %s\n", "Name", "Version", "Path");
    printf("%-16s %-10s %s\n", "----", "-------", "----");
    for (i = 0; i < count; i++) {
        color_on(COL_AMBER);
        printf("%-16s", entries[i].name);
        color_off();
        printf(" %-10s %s\n", entries[i].version, entries[i].path);
    }
    printf("\n%d package%s installed\n",
           count, count > 1 ? "s" : "");
    return 0;
}

/* Timeout for doctor's TCP connection test */
#define HTTP_TIMEOUT_SECS 30

static int cmd_doctor(void)
{
    int all_ok = 1;

    printf("amiport doctor\n");

#ifdef __AMIGA__
    /* Step 1: bsdsocket.library */
    {
        struct Library *lib;
        printf("  bsdsocket.library... ");
        (void)fflush(stdout);
        lib = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 0);
        if (lib) {
            CloseLibrary(lib);
            printf("OK\n");
        } else {
            color_on(COL_RED);
            printf("FAIL");
            color_off();
            printf(" (TCP/IP stack not found. "
                   "Install Roadshow, Miami, or AmiTCP.)\n");
            return 20; /* RETURN_FAIL */
        }
    }
#else
    printf("  bsdsocket.library... OK (host build)\n");
#endif

    /* Step 2: DNS resolution */
    {
        struct hostent *he;
        const char *host = "amiport.platesteel.net";
        printf("  DNS resolution...    ");
        (void)fflush(stdout);

        he = amiport_gethostbyname(host);
        if (he) {
            printf("OK\n");
        } else {
            color_on(COL_RED);
            printf("FAIL");
            color_off();
            printf(" (cannot resolve %s)\n", host);
            all_ok = 0;
        }
    }

    /* Step 3: TCP connection */
    if (all_ok) {
        int sockfd;
        struct sockaddr_in sa;
        struct hostent *he;

        printf("  TCP connection...    ");
        (void)fflush(stdout);

        he = amiport_gethostbyname("amiport.platesteel.net");
        if (he) {
            sockfd = amiport_socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd >= 0) {
                struct amiport_net_timeval tv;
                tv.tv_sec = HTTP_TIMEOUT_SECS;
                tv.tv_usec = 0;
                amiport_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                                   &tv, sizeof(tv));

                memset(&sa, 0, sizeof(sa));
                sa.sin_family = AF_INET;
                sa.sin_port = htons(80);
                memcpy(&sa.sin_addr, he->h_addr, he->h_length);

                if (amiport_connect(sockfd, (struct sockaddr *)&sa,
                                     sizeof(sa)) == 0) {
                    printf("OK\n");
                    amiport_closesocket(sockfd);
                } else {
                    color_on(COL_RED);
                    printf("FAIL");
                    color_off();
                    printf(" (cannot connect to port 80)\n");
                    amiport_closesocket(sockfd);
                    all_ok = 0;
                }
            } else {
                color_on(COL_RED);
                printf("FAIL");
                color_off();
                printf(" (socket creation failed)\n");
                all_ok = 0;
            }
        }
    }

    /* Step 4: HTTP API */
    if (all_ok) {
        char url[256];
        char tmppath[] = "T:amiport-doctor.tmp";
        int http_status = 0;
        int rc;

        printf("  API reachability...  ");
        (void)fflush(stdout);

        snprintf(url, sizeof(url), "%s/api/v1/packages.php", cfg.server);
        rc = amiport_http_get(url, tmppath, &http_status, NULL);
        remove(tmppath);

        if (rc == 0) {
            printf("OK\n");
        } else {
            color_on(COL_RED);
            printf("FAIL");
            color_off();
            printf(" (HTTP %d from %s)\n", http_status, cfg.server);
            all_ok = 0;
        }
    }

    if (all_ok) {
        color_on(COL_GREEN);
        printf("All checks passed.\n");
        color_off();
        return 0;
    }

    return 10;
}

/* --- Cleanup --- */

static void cleanup(void)
{
    (void)fflush(stdout);
    (void)fflush(stderr);
}

/* --- Main --- */

int main(int argc, char *argv[])
{
    int rc;

    atexit(cleanup);

    /* Load config before anything else */
    amiport_config_load(&cfg);

    /* Detect color support */
#ifdef __AMIGA__
    use_color = cfg.color && IsInteractive(Output());
#else
    use_color = cfg.color;
#endif

    /* Suppress AmigaDOS volume requesters */
#ifdef __AMIGA__
    {
        struct Process *me = (struct Process *)FindTask(NULL);
        me->pr_WindowPtr = (APTR)-1L;
    }
#endif

    if (argc < 2) {
        cmd_help();
        return 0;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        cmd_help();
        rc = 0;
    } else if (strcmp(argv[1], "list") == 0) {
        rc = cmd_list();
    } else if (strcmp(argv[1], "search") == 0) {
        rc = cmd_search(argc > 2 ? argv[2] : NULL);
    } else if (strcmp(argv[1], "info") == 0) {
        rc = cmd_info(argc > 2 ? argv[2] : NULL);
    } else if (strcmp(argv[1], "install") == 0) {
        rc = cmd_install(argc > 2 ? argv[2] : NULL);
    } else if (strcmp(argv[1], "upgrade") == 0) {
        rc = cmd_upgrade(argc > 2 ? argv[2] : NULL);
    } else if (strcmp(argv[1], "remove") == 0) {
        rc = cmd_remove(argc > 2 ? argv[2] : NULL);
    } else if (strcmp(argv[1], "installed") == 0) {
        rc = cmd_installed();
    } else if (strcmp(argv[1], "doctor") == 0) {
        rc = cmd_doctor();
    } else {
        fprintf(stderr, "amiport: unknown command '%s'\n", argv[1]);
        fprintf(stderr, "Run 'amiport help' for usage.\n");
        rc = 10;
    }

    return rc;
}
