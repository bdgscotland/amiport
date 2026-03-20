/*
 * test_mmap.c — Tests for amiport Tier 2 mmap emulation
 */

#include "test_framework.h"
#include <amiport/unistd.h>
#include <amiport-emu/mmap.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_mmap 1.0 (20.03.2026)";
long __stack = 32768;

/* Helper: create a test file with known content */
static void create_test_file(const char *path, const char *content)
{
    int fd = amiport_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        amiport_write(fd, content, strlen(content));
        amiport_close(fd);
    }
}

TEST(mmap_read_only)
{
    const char *test_data = "Hello, mmap!";
    int fd;
    void *ptr;

    create_test_file("T:test_mmap.txt", test_data);

    fd = amiport_open("T:test_mmap.txt", O_RDONLY);
    ASSERT(fd >= 0);

    ptr = amiport_emu_mmap(NULL, strlen(test_data), AMIPORT_EMU_PROT_READ,
                            AMIPORT_EMU_MAP_PRIVATE, fd, 0);
    ASSERT(ptr != AMIPORT_EMU_MAP_FAILED);
    ASSERT(memcmp(ptr, test_data, strlen(test_data)) == 0);

    ASSERT_EQ(amiport_emu_munmap(ptr, strlen(test_data)), 0);
    amiport_close(fd);
    amiport_unlink("T:test_mmap.txt");
}

TEST(mmap_anon)
{
    void *ptr;

    ptr = amiport_emu_mmap(NULL, 1024, AMIPORT_EMU_PROT_READ | AMIPORT_EMU_PROT_WRITE,
                            AMIPORT_EMU_MAP_PRIVATE | AMIPORT_EMU_MAP_ANON, -1, 0);
    ASSERT(ptr != AMIPORT_EMU_MAP_FAILED);

    /* Should be writable */
    memset(ptr, 0xAA, 1024);
    ASSERT(((unsigned char *)ptr)[0] == 0xAA);
    ASSERT(((unsigned char *)ptr)[1023] == 0xAA);

    ASSERT_EQ(amiport_emu_munmap(ptr, 1024), 0);
}

TEST(mmap_shared_fails)
{
    void *ptr;
    int fd;

    create_test_file("T:test_mmap2.txt", "test");
    fd = amiport_open("T:test_mmap2.txt", O_RDONLY);
    ASSERT(fd >= 0);

    ptr = amiport_emu_mmap(NULL, 4, AMIPORT_EMU_PROT_READ,
                            AMIPORT_EMU_MAP_SHARED, fd, 0);
    ASSERT(ptr == AMIPORT_EMU_MAP_FAILED);

    amiport_close(fd);
    amiport_unlink("T:test_mmap2.txt");
}

TEST(munmap_invalid)
{
    ASSERT_EQ(amiport_emu_munmap(NULL, 100), -1);
}

int main(void)
{
    (void)verstag;
    printf("=== mmap Emulation Tests ===\n");

    RUN_TEST(mmap_read_only);
    RUN_TEST(mmap_anon);
    RUN_TEST(mmap_shared_fails);
    RUN_TEST(munmap_invalid);

    return test_summary();
}
