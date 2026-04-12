/* test-touch-reftimestamp.rexx -- -r with -t combined: -t wins over -r */
/* Usage: rx WORK:test-touch-reftimestamp.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Verifies: -t 202401010000 overrides -r reference; file created */
OPTIONS FAILAT 21

reffile = 'T:touch_rt_ref_001.txt'
tfile   = 'T:touch_rt_dst_001.txt'

/* Clean up leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || reffile
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Create reference file */
IF OPEN('sf', reffile, 'W') THEN DO
    CALL WRITELN('sf', 'ref')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create reference file'
    EXIT 10
END

/* Touch: -t after -r; last -t wins in getopt processing */
/* Per POSIX, last timestamp-setting flag wins */
ADDRESS COMMAND 'WORK:touch -r ' || reffile || ' -t 202401010000 ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -r -t returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -r -t did not create file'
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    EXIT 10
END
