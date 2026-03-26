/* test-col-run.rexx -- ARexx wrapper to feed stdin to col for FS-UAE testing
 *
 * col reads from STDIN_FILENO only -- no file input argument accepted.
 * ARexx ADDRESS COMMAND does not support stdin piping or < redirection in CMD
 * lines. This wrapper creates an Execute script with the < redirect internally,
 * runs col, then relays its stdout so the test runner can capture it.
 *
 * Usage: rx WORK:test-col-run.rexx <inputfile> [col-flags...]
 *   inputfile  -- FIRST argument must be the file to feed as stdin (WORK: path)
 *   col-flags  -- remaining arguments are passed directly to col
 *                 (e.g., -b, -f, -h, -x, -l 10)
 *
 * Exits with the same RC as col.
 *
 * Examples:
 *   rx WORK:test-col-run.rexx WORK:test-col-plain.txt
 *   rx WORK:test-col-run.rexx WORK:test-col-bold.txt -b
 *   rx WORK:test-col-run.rexx WORK:test-col-spaces.txt -x
 *   rx WORK:test-col-run.rexx WORK:test-col-halflf.txt -f
 *   rx WORK:test-col-run.rexx WORK:test-col-stress.txt -l 500
 */
OPTIONS FAILAT 21

/* Parse: first token is inputfile, rest are col flags */
PARSE ARG inputfile colflags

inputfile = STRIP(inputfile)
colflags = STRIP(colflags)

IF inputfile = '' THEN DO
    SAY 'test-col-run: usage: rx WORK:test-col-run.rexx <inputfile> [col-flags]'
    EXIT 20
END

/* Build the col command -- flags come BEFORE the < redirect */
colcmd = 'WORK:col'
IF colflags ~= '' THEN colcmd = colcmd || ' ' || colflags

/* Temp files */
scriptfile = 'T:col_run_cmd.txt'
outfile    = 'T:col_run_out.txt'
rcfile     = 'T:col_run_rc.txt'

/* Write an Execute script that redirects stdin from inputfile.
 * AmigaDOS Execute supports < for stdin redirect.
 * We use Echo $RC inside the script to capture col's return code. */
IF ~OPEN('sf', scriptfile, 'W') THEN DO
    SAY 'test-col-run: cannot write script file'
    EXIT 20
END
CALL WRITELN('sf', 'FailAt 21')
CALL WRITELN('sf', colcmd '<' inputfile '>' outfile)
CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
CALL CLOSE('sf')

/* Run the script (synchronous -- Execute waits for completion) */
ADDRESS COMMAND 'Execute' scriptfile

/* Read return code from rcfile */
cmdrc = 0
IF OPEN('rcf', rcfile, 'R') THEN DO
    rcline = READLN('rcf')
    CALL CLOSE('rcf')
    IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
END

/* Relay col stdout to our stdout so the test runner captures it */
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        SAY line
    END
    CALL CLOSE('of')
END

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' scriptfile
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile

EXIT cmdrc
