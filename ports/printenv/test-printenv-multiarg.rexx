/* test-printenv-multiarg.rexx -- Test that only the first arg is used     */
/* Sets AMIPORT_FIRST=firstval and AMIPORT_SECOND=secondval, runs          */
/* printenv AMIPORT_FIRST AMIPORT_SECOND. OpenBSD behavior: only first arg */
/* is used -- output should be "firstval" only, RC=0.                     */
/* Usage: rx test-printenv-multiarg.rexx                                   */
OPTIONS FAILAT 21

outfile = 'T:pe_multi_out.txt'
rcfile  = 'T:pe_multi_rc.txt'
script  = 'T:pe_multi_cmd.txt'

/* Set both variables */
ADDRESS COMMAND 'SetEnv AMIPORT_FIRST firstval'
ADDRESS COMMAND 'SetEnv AMIPORT_SECOND secondval'

/* Write execute script: run printenv with two args */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_FIRST AMIPORT_SECOND >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variables */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_FIRST'
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_SECOND'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Read stdout and relay it */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        SAY line
    END
    CALL CLOSE('of')
END

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
