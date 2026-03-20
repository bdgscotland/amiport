/*
 * wc — word, line, and byte count (AmigaOS port)
 *
 * Ported to AmigaOS 3.x by amiport.
 * Original: BSD-style wc implementation.
 *
 * Usage: wc [-clw] [file ...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* amiport: replaced <unistd.h> with amiport shim */
#include <amiport/unistd.h>
/* amiport: replaced <getopt.h> with amiport shim */
#include <amiport/getopt.h>

/* amiport: Amiga version string */
static const char *verstag = "$VER: wc 1.0 (19.03.2026)";

/* amiport: Stack size — 32KB should be plenty for wc */
long __stack = 32768;

/* Count flags */
#define COUNT_LINES  0x01
#define COUNT_WORDS  0x02
#define COUNT_BYTES  0x04

static unsigned int flags = 0;

static long total_lines = 0;
static long total_words = 0;
static long total_bytes = 0;

static void count(FILE *fp, const char *name)
{
    long lines = 0, words = 0, bytes = 0;
    int ch;
    int in_word = 0;

    while ((ch = fgetc(fp)) != EOF) {
        bytes++;
        if (ch == '\n') {
            lines++;
        }
        if (isspace(ch)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }

    if (flags & COUNT_LINES) {
        printf("%8ld", lines);
    }
    if (flags & COUNT_WORDS) {
        printf("%8ld", words);
    }
    if (flags & COUNT_BYTES) {
        printf("%8ld", bytes);
    }
    if (name != NULL) {
        printf(" %s", name);
    }
    printf("\n");

    total_lines += lines;
    total_words += words;
    total_bytes += bytes;
}

int main(int argc, char *argv[])
{
    int ch;
    int file_count = 0;

    /* amiport: using amiport_getopt (via macro) */
    while ((ch = getopt(argc, argv, "clw")) != -1) {
        switch (ch) {
            case 'c':
                flags |= COUNT_BYTES;
                break;
            case 'l':
                flags |= COUNT_LINES;
                break;
            case 'w':
                flags |= COUNT_WORDS;
                break;
            default:
                fprintf(stderr, "usage: wc [-clw] [file ...]\n");
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    /* Default: count everything */
    if (flags == 0) {
        flags = COUNT_LINES | COUNT_WORDS | COUNT_BYTES;
    }

    if (argc == 0) {
        /* Read from stdin */
        count(stdin, NULL);
    } else {
        /* Read from files */
        int i;
        for (i = 0; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "wc: %s: No such file or directory\n", argv[i]);
                continue;
            }
            count(fp, argv[i]);
            fclose(fp);
            file_count++;
        }

        /* Print totals if multiple files */
        if (file_count > 1) {
            if (flags & COUNT_LINES) {
                printf("%8ld", total_lines);
            }
            if (flags & COUNT_WORDS) {
                printf("%8ld", total_words);
            }
            if (flags & COUNT_BYTES) {
                printf("%8ld", total_bytes);
            }
            printf(" total\n");
        }
    }

    return 0;
}
