/* test-printenv-noargs-language.rexx -- no-args lists Language var        */
/* Runs printenv with no args. The Startup-Sequence sets Language=english  */
/* via SetEnv, so this variable should appear in the output.               */
/* Verifies: (a) no-args mode works, (b) output contains Language=english. */
/* Usage: rx test-printenv-noargs-language.rexx                            */
OPTIONS FAILAT 21

outfile = 'T:pe_lang_out.txt'
rcfile  = 'T:pe_lang_rc.txt'
script  = 'T:pe_lang_cmd.txt'

/* Write execute script: run printenv with no args */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv >' || outfile)
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

/* Scan output for Language=english */
found = 0
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF line = 'Language=english' THEN found = 1
    END
    CALL CLOSE('of')
END

/* Report: output the marker if found */
IF found THEN
    SAY 'Language=english'
ELSE
    SAY 'NOT_FOUND'

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
