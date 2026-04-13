/* test-amigit-inrepo-setup.rexx -- CD into repo, create a file, then run amigit
 *
 * Usage:
 *   rx WORK:test-amigit-inrepo-setup.rexx <repo-path> <filename> <subcmd> [args...]
 *
 * Creates <filename> (with fixed content "Hello, Amiga!") in the repo
 * working tree, then runs amigit <subcmd> [args...] from within the repo.
 * Captures stdout and the return code exactly as test-amigit-inrepo.rexx.
 *
 * The "X:foo" -> "X:/foo" rewrite is applied to repo-path before CD so
 * the AmigaDOS path matches the libgit2 path convention used by amigit.
 *
 * Rules followed:
 *   - Pure ASCII
 *   - Single-level stem variables only
 *   - ~= for not-equal
 *   - OPTIONS FAILAT 21 at top
 */

OPTIONS FAILAT 21

PARSE ARG repopath filename rest

IF repopath = '' THEN DO
    SAY 'test-amigit-inrepo-setup: missing repo-path argument'
    EXIT 10
END

IF filename = '' THEN DO
    SAY 'test-amigit-inrepo-setup: missing filename argument'
    EXIT 10
END

subcmd = STRIP(rest)
IF subcmd = '' THEN DO
    SAY 'test-amigit-inrepo-setup: missing subcommand argument'
    EXIT 10
END

outfile    = 'T:amigit_setup_out.txt'
rcfile     = 'T:amigit_setup_rc.txt'
scriptfile = 'T:amigit_setup_script.txt'

/* Remove stale files from previous runs */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile

/* Rewrite "X:foo" -> "X:/foo" so the CD path matches libgit2 convention */
cdpath = repopath
IF LENGTH(cdpath) > 2 & SUBSTR(cdpath, 2, 1) = ':' & SUBSTR(cdpath, 3, 1) ~= '/' THEN
    cdpath = LEFT(cdpath, 2) || '/' || SUBSTR(cdpath, 3)

/* Write the Execute script:
 *   Stack 262144  -- match amigit __stack cookie
 *   FailAt 21     -- prevent errors from aborting the script silently
 *   CD into repo
 *   Echo a line into the file (creates / overwrites the file)
 *   Run amigit subcommand, redirect stdout to outfile
 *   Capture RC to rcfile
 */
IF OPEN('sf', scriptfile, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 262144')
    CALL WRITELN('sf', 'CD ' || cdpath)
    CALL WRITELN('sf', 'Echo >' || filename || ' "Hello, Amiga!"')
    CALL WRITELN('sf', 'WORK:amigit ' || subcmd || ' >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'test-amigit-inrepo-setup: cannot write script file'
    EXIT 10
END

/* Execute the script */
ADDRESS COMMAND 'Execute ' || scriptfile

/* Read amigit return code */
cmdrc = 0
IF OPEN('rcf', rcfile, 'R') THEN DO
    rcline = READLN('rcf')
    CALL CLOSE('rcf')
    IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
END

/* SAY every line of amigit stdout so the test harness captures them */
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

/* Exit with amigit real RC so EXPECT_RC assertions work */
EXIT cmdrc
