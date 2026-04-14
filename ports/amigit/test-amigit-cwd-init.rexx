/* test-amigit-cwd-init.rexx -- diagnose amigit init from a CWD
 *
 * Creates T:cwd-init-diag, CDs into it, runs WORK:amigit init with no
 * arguments, captures stdout/stderr merged + AmigaDOS $RC, echoes the
 * lot back so the test harness can show it.  The goal is to reproduce
 * the bug the user hit where plain "amigit init" from a playground
 * CWD failed with "failed to stat '.'", since every existing init test
 * uses an explicit T:/amigit-test path.
 *
 * Exits with amigit's own return code so EXPECT_RC assertions work.
 */

OPTIONS FAILAT 21

outfile    = 'T:amigit_cwdinit_out.txt'
rcfile     = 'T:amigit_cwdinit_rc.txt'
scriptfile = 'T:amigit_cwdinit_script.txt'
testdir    = 'T:/cwd-init-diag'

/* Wipe any prior fixture so init exercises the create path. */
ADDRESS COMMAND 'Delete >NIL: ALL FORCE ' || testdir
ADDRESS COMMAND 'MakeDir ' || testdir

/* Remove stale output files. */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile

/* Write an Execute script that CDs into the test dir then runs
 * amigit init (no args) with stdout+stderr merged to the output file.
 * Use the libnix path form "T:/cwd-init-diag" to match what cmd_init
 * hands to libgit2 (see amigit_resolve_repo_path docs).
 */
IF OPEN('sf', scriptfile, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 262144')
    CALL WRITELN('sf', 'CD ' || testdir)
    /* AmigaDOS does NOT parse `2>&1` -- it gets passed as a literal
     * positional arg to amigit. Use plain `>` (stdout only). stderr
     * goes to the ANSI-capture log via console.device for diagnostics. */
    CALL WRITELN('sf', 'WORK:amigit init >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'test-amigit-cwd-init: cannot write script file'
    EXIT 10
END

ADDRESS COMMAND 'Execute ' || scriptfile

/* Read back amigit's RC */
cmdrc = 0
IF OPEN('rcf', rcfile, 'R') THEN DO
    rcline = READLN('rcf')
    CALL CLOSE('rcf')
    IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
END

/* SAY every captured output line so the harness records it. */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        SAY line
    END
    CALL CLOSE('of')
END

/* Cleanup the scratch files but leave testdir so a follow-up test
 * (reinit from CWD) can run against it. */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile

EXIT cmdrc
