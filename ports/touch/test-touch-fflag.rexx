/* test-touch-fflag.rexx -- -f flag: silently ignored (BSD compatibility), RC=0 */
/* Usage: rx WORK:test-touch-fflag.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

tfile = 'T:touch_fflag_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch -f: flag is accepted but does nothing (BSD compat stub) */
ADDRESS COMMAND 'WORK:touch -f ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -f returned RC=' || trc || ' (expected 0)'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -f did not create file'
    EXIT 10
END
