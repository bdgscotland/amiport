/* test-printenv-noargs-format.rexx -- Verify NAME=VALUE format for all   */
/* Sets AMIPORT_FORMAT=fmttest, runs printenv with no args, verifies that */
/* the specific line "AMIPORT_FORMAT=fmttest" appears (NAME=VALUE format).*/
/* Usage: rx test-printenv-noargs-format.rexx                              */
OPTIONS FAILAT 21

outfile = 'T:pe_fmt_out.txt'
rcfile  = 'T:pe_fmt_rc.txt'
script  = 'T:pe_fmt_cmd.txt'

/* Set a known variable */
ADDRESS COMMAND 'SetEnv AMIPORT_FORMAT fmttest'

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_FORMAT'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Scan for exactly "AMIPORT_FORMAT=fmttest" (correct NAME=VALUE format) */
found = 0
wrongfmt = 0
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF line = 'AMIPORT_FORMAT=fmttest' THEN found = 1
        /* Check no line starts with just the value without NAME= */
        IF line = 'fmttest' THEN wrongfmt = 1
    END
    CALL CLOSE('of')
END

IF found & ~wrongfmt THEN
    SAY 'AMIPORT_FORMAT=fmttest'
ELSE IF wrongfmt THEN
    SAY 'WRONG_FORMAT_NO_EQUALS'
ELSE
    SAY 'NOT_FOUND'

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
