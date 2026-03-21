/* Fixture: uses stat, getopt, getline, strlcpy, pledge, err */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;

    if (pledge("stdio rpath", NULL) == -1)
        err(1, "pledge");

    while ((ch = getopt(argc, argv, "n")) != -1)
        ;

    if (stat(argv[1], &sb) != 0)
        errx(1, "stat failed");

    while (getline(&line, &len, stdin) != -1) {
        strlcpy(buf, line, sizeof(buf));
        printf("%s", buf);
    }

    free(line);
    return 0;
}
