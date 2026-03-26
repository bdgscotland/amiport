/* test-printenv-noargs.rexx -- Run printenv with no args, verify output   */
/* Sets AMIPORT_VERIFY=verifyval, runs printenv with no args,              */
/* checks that AMIPORT_VERIFY=verifyval appears in output.                 */
/* Usage: rx test-printenv-noargs.rexx                                     */
OPTIONS FAILAT 21

outfile = 'T:pe_noargs_out.txt'
rcfile  = 'T:pe_noargs_rc.txt'
script  = 'T:pe_noargs_cmd.txt'

/* Set a known variable we can search for */
ADDRESS COMMAND 'SetEnv AMIPORT_VERIFY verifyval'

/* Write execute script: run printenv with no args */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove the test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_VERIFY'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Search output for our known variable */
found = 0
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF line = 'AMIPORT_VERIFY=verifyval' THEN found = 1
    END
    CALL CLOSE('of')
END

/* Report result: SAY the marker line if found, or error if not */
IF found THEN
    SAY 'AMIPORT_VERIFY=verifyval'
ELSE
    SAY 'NOT_FOUND'

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
