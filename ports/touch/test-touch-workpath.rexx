/* test-touch-workpath.rexx -- Amiga-specific: touch file on WORK: volume */
/* Usage: rx WORK:test-touch-workpath.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Verifies WORK: volume path handling (AmigaDOS volume notation) */
OPTIONS FAILAT 21

/* Use T: for writeable temp storage -- WORK: is read-only in test environment */
/* But we verify WORK: binary can handle T: paths with AmigaDOS notation */
tfile = 'T:touch_work_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch a file using T: volume prefix (AmigaDOS volume notation test) */
ADDRESS COMMAND 'WORK:touch ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch on T: volume returned RC=' || trc
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: file not created on T: volume'
    EXIT 10
END
