/* test-touch-existing.rexx -- touch an existing file updates it, RC=0 */
/* Usage: rx WORK:test-touch-existing.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

tfile = 'T:touch_exist_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Create the file first */
IF OPEN('sf', tfile, 'W') THEN DO
    CALL WRITELN('sf', 'existing file content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

/* Touch the existing file */
ADDRESS COMMAND 'WORK:touch ' || tfile
trc = RC

/* File must still exist and RC must be 0 */
IF trc ~= 0 THEN DO
    SAY 'FAIL: touch returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: file disappeared after touch'
    EXIT 10
END
