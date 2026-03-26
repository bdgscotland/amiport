/* test-printenv-secondarg.rexx -- Test that second arg is ignored when    */
/* first arg exists. Sets AMIPORT_ONLY=onlyme, runs:                      */
/* printenv AMIPORT_ONLY AMIPORT_NOEXIST_ZZZZZ                            */
/* Expected: "onlyme" printed, RC=0 (first arg found, second ignored).    */
/* Usage: rx test-printenv-secondarg.rexx                                  */
OPTIONS FAILAT 21

outfile = 'T:pe_second_out.txt'
rcfile  = 'T:pe_second_rc.txt'
script  = 'T:pe_second_cmd.txt'

/* Set first variable only */
ADDRESS COMMAND 'SetEnv AMIPORT_ONLY onlyme'
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_NOEXIST_ZZZZZ'

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_ONLY AMIPORT_NOEXIST_ZZZZZ >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_ONLY'

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
