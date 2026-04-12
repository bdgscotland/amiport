/* test-touch-combined.rexx -- combined -m -t flags: mtime-only with timestamp */
/* Usage: rx WORK:test-touch-combined.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

tfile = 'T:touch_comb_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch -m -t: set mtime to specific timestamp, ignore atime */
ADDRESS COMMAND 'WORK:touch -m -t 202309150830 ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -m -t returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -m -t did not create file'
    EXIT 10
END
