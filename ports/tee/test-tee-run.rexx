/* test-tee-run.rexx -- ARexx wrapper to feed stdin to tee for FS-UAE testing
 *
 * tee reads from STDIN_FILENO and cannot accept a file as an input argument.
 * ARexx ADDRESS COMMAND does not support stdin piping or < redirection in CMD
 * lines. This wrapper creates an Execute script with the < redirect internally,
 * runs tee, then relays its stdout so the test runner can capture it.
 *
 * Usage: rx WORK:test-tee-run.rexx <inputfile> [tee-args...]
 *   inputfile  -- FIRST argument must be the file to feed as stdin (WORK: path)
 *   tee-args   -- remaining arguments are passed directly to tee
 *                 (flags like -a, output file paths, etc.)
 *
 * Exits with the same RC as tee.
 *
 * Examples:
 *   rx WORK:test-tee-run.rexx WORK:test-oneline.txt
 *   rx WORK:test-tee-run.rexx WORK:test-oneline.txt T:tee-out.txt
 *   rx WORK:test-tee-run.rexx WORK:test-oneline.txt -a T:tee-out.txt
 *   rx WORK:test-tee-run.rexx WORK:test-oneline.txt T:out1.txt T:out2.txt
 */
OPTIONS FAILAT 21

/* Parse: first token is inputfile, rest are tee args */
PARSE ARG inputfile teeargs

inputfile = STRIP(inputfile)
teeargs = STRIP(teeargs)

IF inputfile = '' THEN DO
    SAY 'test-tee-run: usage: rx WORK:test-tee-run.rexx <inputfile> [tee-args]'
    EXIT 20
END

/* Build the tee command */
teecmd = 'WORK:tee'
IF teeargs ~= '' THEN teecmd = teecmd || ' ' || teeargs

/* Temp files */
scriptfile = 'T:tee_run_cmd.txt'
outfile = 'T:tee_run_out.txt'
rcfile = 'T:tee_run_rc.txt'

/* Write an Execute script that redirects stdin from inputfile */
IF ~OPEN('sf', scriptfile, 'W') THEN DO
    SAY 'test-tee-run: cannot write script file'
    EXIT 20
END
CALL WRITELN('sf', 'FailAt 21')
CALL WRITELN('sf', teecmd '<' inputfile '>' outfile)
CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
CALL CLOSE('sf')

/* Run the script */
ADDRESS COMMAND 'Execute' scriptfile

/* Read return code */
cmdrc = 0
IF OPEN('rcf', rcfile, 'R') THEN DO
    rcline = READLN('rcf')
    CALL CLOSE('rcf')
    IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
END

/* Relay tee stdout to our stdout so the test runner captures it */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        SAY line
    END
    CALL CLOSE('of')
END

/* Clean up */
ADDRESS COMMAND 'Delete >NIL:' scriptfile
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile

EXIT cmdrc
