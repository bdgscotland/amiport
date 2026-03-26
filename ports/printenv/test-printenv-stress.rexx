/* test-printenv-stress.rexx -- Stress test: set 20 vars, list all, check */
/* Sets 20 environment variables, runs printenv with no args, verifies     */
/* all 20 appear in output, then cleans up. Tests ExAll() iteration when   */
/* ENV: has many entries.                                                   */
/* Usage: rx test-printenv-stress.rexx                                     */
OPTIONS FAILAT 21

outfile = 'T:pe_stress_out.txt'
rcfile  = 'T:pe_stress_rc.txt'
script  = 'T:pe_stress_cmd.txt'

/* Set 20 test variables */
ADDRESS COMMAND 'SetEnv APETEST01 val01'
ADDRESS COMMAND 'SetEnv APETEST02 val02'
ADDRESS COMMAND 'SetEnv APETEST03 val03'
ADDRESS COMMAND 'SetEnv APETEST04 val04'
ADDRESS COMMAND 'SetEnv APETEST05 val05'
ADDRESS COMMAND 'SetEnv APETEST06 val06'
ADDRESS COMMAND 'SetEnv APETEST07 val07'
ADDRESS COMMAND 'SetEnv APETEST08 val08'
ADDRESS COMMAND 'SetEnv APETEST09 val09'
ADDRESS COMMAND 'SetEnv APETEST10 val10'
ADDRESS COMMAND 'SetEnv APETEST11 val11'
ADDRESS COMMAND 'SetEnv APETEST12 val12'
ADDRESS COMMAND 'SetEnv APETEST13 val13'
ADDRESS COMMAND 'SetEnv APETEST14 val14'
ADDRESS COMMAND 'SetEnv APETEST15 val15'
ADDRESS COMMAND 'SetEnv APETEST16 val16'
ADDRESS COMMAND 'SetEnv APETEST17 val17'
ADDRESS COMMAND 'SetEnv APETEST18 val18'
ADDRESS COMMAND 'SetEnv APETEST19 val19'
ADDRESS COMMAND 'SetEnv APETEST20 val20'

/* Write execute script: run printenv with no args */
IF OPEN('sf', script, 'W') THEN DO
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'Stack 65536')
    CALL WRITELN('sf', 'WORK:printenv >' || outfile)
    CALL WRITELN('sf', 'Echo >' || rcfile || ' $RC')
    CALL CLOSE('sf')
END

ADDRESS COMMAND 'Execute' script

/* Remove all 20 test variables */
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST01'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST02'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST03'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST04'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST05'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST06'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST07'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST08'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST09'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST10'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST11'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST12'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST13'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST14'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST15'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST16'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST17'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST18'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST19'
ADDRESS COMMAND 'Delete >NIL: ENV:APETEST20'

/* Read printenv return code */
prc = 0
IF OPEN('rf', rcfile, 'R') THEN DO
    rcline = READLN('rf')
    CALL CLOSE('rf')
    IF DATATYPE(STRIP(rcline), 'W') THEN prc = STRIP(rcline)
END

/* Count how many of our 20 variables appear in output */
found = 0
IF OPEN('of', outfile, 'R') THEN DO
    DO WHILE ~EOF('of')
        line = READLN('of')
        IF LEFT(line, 8) = 'APETEST0' THEN found = found + 1
        IF LEFT(line, 8) = 'APETEST1' THEN found = found + 1
        IF LEFT(line, 8) = 'APETEST2' THEN found = found + 1
    END
    CALL CLOSE('of')
END

/* Report: SAY the count */
SAY 'found=' || found

/* Clean up temp files */
ADDRESS COMMAND 'Delete >NIL:' outfile
ADDRESS COMMAND 'Delete >NIL:' rcfile
ADDRESS COMMAND 'Delete >NIL:' script

EXIT prc
