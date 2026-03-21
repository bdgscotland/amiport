/* Fixture: uses fork, execvp, waitpid, setenv, getenv — Tier 3 heavy */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    pid_t pid;

    setenv("PATH", "/usr/bin", 1);
    char *path = getenv("PATH");

    pid = fork();
    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        err(1, "execvp");
    }

    int status;
    waitpid(pid, &status, 0);
    printf("exit: %d\n", status);
    return 0;
}
