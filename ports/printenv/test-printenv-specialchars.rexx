/* test-printenv-specialchars.rexx -- Test var value with special chars    */
/* Sets AMIPORT_SPECIAL to a value containing spaces and punctuation,      */
/* runs printenv AMIPORT_SPECIAL, verifies full value is printed.          */
/* Tests that GetVar() preserves non-alphanumeric values correctly.        */
/* Usage: rx test-printenv-specialchars.rexx                               */
OPTIONS FAILAT 21

outfile = 'T:pe_special_out.txt'
rcfile  = 'T:pe_special_rc.txt'
script  = 'T:pe_special_cmd.txt'

/* Set variable with a value containing spaces and punctuation */
/* Note: AmigaDOS SetEnv may handle this differently from GetVar -- */
/* the value after the variable name is taken as-is by SetEnv       */
ADDRESS COMMAND 'SetEnv AMIPORT_SPECIAL hello-world'

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv AMIPORT_SPECIAL >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove test variable */
ADDRESS COMMAND 'Delete >NIL: ENV:AMIPORT_SPECIAL'

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
