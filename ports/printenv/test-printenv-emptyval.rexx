/* test-printenv-emptyval.rexx -- Test printenv with empty-value variable  */
/* Sets AMIPORT_EMPTY= (empty string), runs printenv AMIPORT_EMPTY.        */
/* Should print an empty line and exit 0 (variable exists, value is empty).*/
/* Usage: rx test-printenv-emptyval.rexx                                   */
OPTIONS FAILAT 21

outfile = 'T:pe_empty_out.txt'
rcfile  = 'T:pe_empty_rc.txt'
script  = 'T:pe_empty_cmd.txt'

/* Set variable to empty string */
ADDRESS COMMAND 'SetEnv AMIPORT_EMPTY '

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_EMPTY >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_EMPTY'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Read stdout and relay it */
linecount = 0
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        linecount = linecount + 1
        SAY line
    END
    CALL CLOSE('of')
END

/* If no output at all, emit a marker so the test can detect it */
IF linecount = 0 THEN SAY 'EMPTY_OUTPUT'

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
