/* test-which-nopath.rexx -- Run WORK:which with PATH env var unset */
/* Ensures PATH env var does not exist so which falls back to _PATH_DEFPATH */
/* The fallback path is "C:" which (via strsep) searches "C" and "." */
/* On AmigaOS, "C" is not a volume prefix without ":", so commands */
/* are NOT found via the default fallback -- this tests that behavior. */
/* Usage: rx test-which-nopath.rexx [which-args...] */
OPTIONS FAILAT 21
PARSE ARG args
args = STRIP(args)

outfile = 'T:which_np_out.txt'
rcfile  = 'T:which_np_rc.txt'
script  = 'T:which_np_cmd.txt'

/* Remove PATH env var if it exists */
ADDRESS COMMAND 'Delete >NIL: ENV:PATH'

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
        SAY line
    END
    CALL CLOSE('of')
END

/* Clean up */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT whichrc
