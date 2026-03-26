/* test-printenv-setvar.rexx -- Set a known env var, run printenv, cleanup */
/* Sets AMIPORT_TEST=testvalue in ENV:, runs printenv AMIPORT_TEST,         */
/* prints result, removes var, exits with printenv's RC.                    */
/* Usage: rx test-printenv-setvar.rexx                                      */
OPTIONS FAILAT 21

outfile = 'T:pe_setvar_out.txt'
rcfile  = 'T:pe_setvar_rc.txt'
script  = 'T:pe_setvar_cmd.txt'

/* Set a known variable in the global environment */
ADDRESS COMMAND 'SetEnv AMIPORT_TEST testvalue'

/* Write execute script: runs printenv, captures stdout and RC */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_TEST >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove the test env var -- do not pollute other tests */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_TEST'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Read printenv stdout and relay it */
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
