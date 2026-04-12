/* test-touch-mflag.rexx -- -m flag: update mtime only, file created if missing */
/* Usage: rx WORK:test-touch-mflag.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

tfile = 'T:touch_mflag_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch -m: creates file, updates mtime only */
ADDRESS COMMAND 'WORK:touch -m ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -m returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -m did not create file'
    EXIT 10
END
