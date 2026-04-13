/* test-amigit-inrepo.rexx -- Run amigit in a specific repo directory
 *
 * Usage: rx WORK:test-amigit-inrepo.rexx <repo-path> <subcmd> [arg...]
 *
 * Builds a small AmigaDOS Execute script that CDs into <repo-path>
 * then runs amigit <subcmd> [arg...], capturing output to T:.  After
 * execution the output lines are SAY'd so the test harness can capture
 * them via the > redirect applied to this rx process.
 *
 * The Execute-script trick is required because AmigaDOS CD within a
 * script persists for the lifetime of that script -- a second ADDRESS
 * COMMAND call from ARexx would spawn a fresh process with the old CWD.
 */

OPTIONS FAILAT 21

PARSE ARG repopath rest

IF repopath = '' THEN DO
    SAY 'test-amigit-inrepo: missing repo-path argument'
    EXIT 10
END

/* Build the command string from the remaining args (subcmd + flags) */
subcmd = STRIP(rest)
IF subcmd = '' THEN DO
    SAY 'test-amigit-inrepo: missing subcommand argument'
    EXIT 10
END

outfile   = 'T:amigit_inrepo_out.txt'
rcfile    = 'T:amigit_inrepo_rc.txt'
scriptfile = 'T:amigit_inrepo_script.txt'

/* Remove stale files from previous runs */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile

/* Write the Execute script:
 *   CD into the repo
 *   Run amigit, redirect stdout to outfile
 *   Capture $RC (AmigaDOS return code variable) to rcfile
 */
/*
 * Rewrite "X:foo" -> "X:/foo" before CD. amigit stores repos using
 * the libnix/newlib path form where AmigaDOS volume names are
 * followed by a slash ("T:/amigit-test"). Plain "T:amigit-test" is
 * NOT the same directory under libnix -- attempting to CD to it
 * returns "object not found". This wrapper must use the same
 * convention as cmd_init.c so the test fixture remains reachable.
 */
cdpath = repopath
IF LENGTH(cdpath) > 2 & SUBSTR(cdpath, 2, 1) = ':' & SUBSTR(cdpath, 3, 1) ~= '/' THEN
    cdpath = LEFT(cdpath, 2) || '/' || SUBSTR(cdpath, 3)

IF OPEN('sf', scriptfile, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 262144')
    CALL WRITELN('sf', 'CD ' || cdpath)
    CALL WRITELN('sf', 'WORK:amigit ' || subcmd || ' >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'test-amigit-inrepo: cannot write script file'
    EXIT 10
END

/* Execute the script */
ADDRESS COMMAND 'Execute ' || scriptfile

/* Read the amigit return code */
cmdrc = 0
IF OPEN('rcf', rcfile, 'R') THEN DO
    rcline = READLN('rcf')
    CALL CLOSE('rcf')
    IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
END

/* SAY every line of amigit's stdout -- test harness captures these */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        SAY line
    END
    CALL CLOSE('of')
END

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile

/* Exit with amigit's actual return code so EXPECT_RC: assertions work */
EXIT cmdrc
