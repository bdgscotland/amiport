/* test-touch-nocreate-existing.rexx -- -c on existing file updates it, RC=0 */
/* Usage: rx WORK:test-touch-nocreate-existing.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* -c only suppresses creation; existing files are updated normally */
OPTIONS FAILAT 21

tfile = 'T:touch_ce_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Create the file first */
IF OPEN('sf', tfile, 'W') THEN DO
    CALL WRITELN('sf', 'pre-existing')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

/* Touch -c on existing file: should update timestamp, file must remain */
ADDRESS COMMAND 'WORK:touch -c ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -c on existing file returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -c removed the existing file'
    EXIT 10
END
