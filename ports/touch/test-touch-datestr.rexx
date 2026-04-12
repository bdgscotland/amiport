/* test-touch-datestr.rexx -- -d ISO 8601 date string, verify file created */
/* Usage: rx WORK:test-touch-datestr.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

tfile = 'T:touch_date_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch with -d ISO 8601 date string */
ADDRESS COMMAND 'WORK:touch -d 2024-06-15T12:00:00 ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -d returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -d did not create file'
    EXIT 10
END
