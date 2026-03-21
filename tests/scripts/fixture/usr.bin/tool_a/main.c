/* Fixture: uses stat, getopt, strlcpy, fprintf, exit */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    struct stat sb;
    int ch;
    char buf[256];

    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'h': break;
        }
    }

    if (stat(argv[1], &sb) != 0) {
        fprintf(stderr, "error\n");
        exit(1);
    }

    strlcpy(buf, argv[1], sizeof(buf));
    printf("%s\n", buf);
    return 0;
}
