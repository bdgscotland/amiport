/* test-which-path.rexx -- Run WORK:which with PATH=C:;WORK: set */
/* Sets the PATH environment variable so which can find commands in C: */
/* and WORK:, then exits with which's RC. */
/* PATH uses AmigaDOS semicolon separator: C:;WORK: */
/* Usage: rx test-which-path.rexx [which-args...] */
OPTIONS FAILAT 21
PARSE ARG args
args = STRIP(args)

outfile = 'T:which_path_out.txt'
rcfile  = 'T:which_path_rc.txt'
script  = 'T:which_path_cmd.txt'

/* Set PATH env var: AmigaDOS semicolon-separated, volume-style entries */
ADDRESS COMMAND 'SetEnv PATH C:;WORK:'

/* Write execute script: runs which with args, captures stdout and RC */
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

/* Remove the PATH env var we set -- do not pollute other tests */
ADDRESS COMMAND 'Delete >NIL: ENV:PATH'

/* Read which's return code */
whichrc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN whichrc = STRIP(rcline)
END

/* Read which's stdout and print it to our stdout */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF line ~= '' THEN SAY line
    END
    CALL CLOSE('of')
END

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT whichrc
