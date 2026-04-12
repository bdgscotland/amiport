/* test-touch-aflag.rexx -- -a flag: accepted on AmigaOS, no effect, RC=0 */
/* Usage: rx WORK:test-touch-aflag.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* NOTE: AmigaOS does not have separate atime; -a accepted but no-op */
OPTIONS FAILAT 21

tfile = 'T:touch_aflag_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch -a: should succeed (create file, atime update silently ignored) */
ADDRESS COMMAND 'WORK:touch -a ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -a returned RC=' || trc || ' (expected 0)'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -a did not create file'
    EXIT 10
END
