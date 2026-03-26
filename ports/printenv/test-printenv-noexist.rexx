/* test-printenv-noexist.rexx -- Run printenv with a nonexistent variable */
/* Verifies that printenv exits 10 when the variable is not set.           */
/* Usage: rx test-printenv-noexist.rexx                                    */
OPTIONS FAILAT 21

outfile = 'T:pe_noexist_out.txt'
rcfile  = 'T:pe_noexist_rc.txt'
script  = 'T:pe_noexist_cmd.txt'

/* Ensure the variable definitely does not exist */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_NOEXIST_ZZZZZ'

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_NOEXIST_ZZZZZ >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Read stdout (should be empty) and relay it */
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
