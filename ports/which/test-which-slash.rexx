/* test-which-slash.rexx -- Run WORK:which with PATH having trailing slash */
/* Tests that PATH entry "WORK:/" has trailing slash stripped before joining. */
/* Source code strips trailing '/' from PATH entries before snprintf. */
/* Usage: rx test-which-slash.rexx [which-args...] */
OPTIONS FAILAT 21
PARSE ARG args
args = STRIP(args)

outfile = 'T:which_sl_out.txt'
rcfile  = 'T:which_sl_rc.txt'
script  = 'T:which_sl_cmd.txt'

/* Set PATH with trailing slash on WORK: entry -- tests slash stripping logic */
ADDRESS COMMAND 'SetEnv PATH WORK:/'

/* Write execute script */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    IF args = '' THEN
        CALL WRITELN('sf', 'WORK:which >' || outfile)
    ELSE
        CALL WRITELN('sf', 'WORK:which' args '>' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove the PATH env var we set */
ADDRESS COMMAND 'Delete >NIL: ENV:PATH'

/* Read return code */
whichrc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN whichrc = STRIP(rcline)
END

/* Print which's stdout */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF line ~= '' THEN SAY line
    END
    CALL CLOSE('of')
END

/* Clean up */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT whichrc
