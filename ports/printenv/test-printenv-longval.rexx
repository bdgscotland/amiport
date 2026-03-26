/* test-printenv-longval.rexx -- Test printenv with a long value (>200 ch) */
/* Sets AMIPORT_LONG to a 200-character value, runs printenv AMIPORT_LONG. */
/* Verifies GetVar() buffer handling for long-ish values (ENV_VAL_BUF=256).*/
/* Usage: rx test-printenv-longval.rexx                                    */
OPTIONS FAILAT 21

outfile = 'T:pe_long_out.txt'
rcfile  = 'T:pe_long_rc.txt'
script  = 'T:pe_long_cmd.txt'

/* 200-char value: 20 repetitions of "0123456789" */
longval = '01234567890123456789012345678901234567890123456789'
longval = longval || '01234567890123456789012345678901234567890123456789'
longval = longval || '01234567890123456789012345678901234567890123456789'
longval = longval || '01234567890123456789'

/* Set the variable with the long value */
ADDRESS COMMAND 'SetEnv AMIPORT_LONG' longval

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_LONG >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_LONG'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Read stdout and relay first line */
IF OPEN('of', outfile, 'R') THEN DO
    line = READLN('of')
    CALL CLOSE('of')
    SAY line
END

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
