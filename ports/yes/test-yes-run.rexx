/* test-yes-run.rexx -- Run yes with timeout, break, capture output
 *
 * Usage: rx WORK:test-yes-run.rexx [args-for-yes]
 *
 * Backgrounds yes via Run, waits briefly, parses the CLI number
 * from Run's output, sends Break C to stop it, then outputs
 * the captured content via SAY.
 *
 * Uses a temp Execute script to isolate yes's > redirect from
 * Run's > redirect (AmigaDOS parses ALL > at the top level).
 */
OPTIONS FAILAT 21

PARSE ARG yesargs
yesargs = STRIP(yesargs)

outfile = 'T:yes_run_out.txt'
clinumfile = 'T:yes_cli_num.txt'
cmdscript = 'T:yes_cmd.txt'

/* Write a temp script with yes + output redirect */
IF OPEN('sf', cmdscript, 'W') THEN DO
    IF yesargs = '' THEN
        CALL WRITELN('sf', 'WORK:yes >' || outfile)
    ELSE
        CALL WRITELN('sf', 'WORK:yes' yesargs '>' || outfile)
    CALL CLOSE('sf')
END

/* Run the script in background, capture CLI number */
ADDRESS COMMAND 'Run >' || clinumfile || ' Execute' cmdscript

/* Wait 1 second for output */
ADDRESS COMMAND 'Wait 1'

/* Parse CLI number and break */
IF OPEN('cf', clinumfile, 'R') THEN DO
    cliline = READLN('cf')
    CALL CLOSE('cf')
    PARSE VAR cliline '[CLI' clinum ']'
    clinum = STRIP(clinum)
    IF DATATYPE(clinum, 'W') THEN
        ADDRESS COMMAND 'Break' clinum 'C'
END

/* Wait for clean exit */
ADDRESS COMMAND 'Wait 1'

/* Read and output captured content */
IF OPEN('of', outfile, 'R') THEN DO
    line = READLN('of')
    SAY line
    CALL CLOSE('of')
END

/* Cleanup */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' clinumfile
ADDRESS COMMAND 'Delete >NIL:' cmdscript

EXIT 0
