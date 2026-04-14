/* test-amigit-inrepo-commit-f.rexx -- stage a file + commit with -F <msgfile>
 *
 * Usage:
 *   rx WORK:test-amigit-inrepo-commit-f.rexx <repo-path> <stage-filename> <msg-content>
 *
 * Purpose: exercise the happy path for `amigit commit -F <file>` by
 * 1) CDing into an existing repo, 2) creating a new file in the working
 * tree, 3) staging it via `amigit add`, 4) writing msg-content to a
 * T: temp file, and 5) running `amigit commit -F T:amigit_f_msg.txt`.
 *
 * The repo must already have at least one commit so the "nothing to
 * commit" early-exit doesn't fire before the -F path runs. The c3
 * fixture created by the main test sequence has two commits by the
 * time this wrapper is invoked (see [ordered-9]).
 *
 * Rules followed:
 *   - Pure ASCII
 *   - Single-level stem variables only
 *   - ~= for not-equal
 *   - OPTIONS FAILAT 21 at top
 */

OPTIONS FAILAT 21

PARSE ARG repopath filename msgcontent

IF repopath = '' THEN DO
    SAY 'test-amigit-inrepo-commit-f: missing repo-path argument'
    EXIT 10
END

IF filename = '' THEN DO
    SAY 'test-amigit-inrepo-commit-f: missing filename argument'
    EXIT 10
END

msgcontent = STRIP(msgcontent)
IF msgcontent = '' THEN DO
    SAY 'test-amigit-inrepo-commit-f: missing message content'
    EXIT 10
END

outfile    = 'T:amigit_fc_out.txt'
rcfile     = 'T:amigit_fc_rc.txt'
scriptfile = 'T:amigit_fc_script.txt'
msgfile    = 'T:amigit_f_msg.txt'

/* Remove stale files from previous runs */
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || rcfile
ADDRESS COMMAND 'Delete >NIL: ' || scriptfile
ADDRESS COMMAND 'Delete >NIL: ' || msgfile

/* Write the commit message file directly from ARexx. */
IF OPEN('mf', msgfile, 'W') THEN DO
    CALL WRITELN('mf', msgcontent)
    CALL CLOSE('mf')
END
ELSE DO
    SAY 'test-amigit-inrepo-commit-f: cannot write message file'
    EXIT 10
END

/* Rewrite "X:foo" -> "X:/foo" so the CD path matches libgit2 convention */
cdpath = repopath
IF LENGTH(cdpath) > 2 & SUBSTR(cdpath, 2, 1) = ':' & SUBSTR(cdpath, 3, 1) ~= '/' THEN
    cdpath = LEFT(cdpath, 2) || '/' || SUBSTR(cdpath, 3)

/* Write the Execute script:
 *   Stack 262144  -- match amigit __stack cookie
 *   FailAt 21     -- prevent errors from aborting the script silently
 *   CD into repo
 *   Echo a line into the staged file (creates/overwrites)
 *   Run amigit add <filename>
 *   Run amigit commit -F <msgfile>, redirect stdout to outfile
 *   Capture RC to rcfile
 */
IF OPEN('sf', scriptfile, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 262144')
    CALL WRITELN('sf', 'CD ' || cdpath)
    CALL WRITELN('sf', 'Echo >' || filename || ' "Hello, amigit -F!"')
    CALL WRITELN('sf', 'WORK:amigit add ' || filename)
    CALL WRITELN('sf', 'WORK:amigit commit -F ' || msgfile || ' >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'test-amigit-inrepo-commit-f: cannot write script file'
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
ADDRESS COMMAND 'Delete >NIL: ' || msgfile

/* Exit with amigit real RC so EXPECT_RC assertions work */
EXIT cmdrc
