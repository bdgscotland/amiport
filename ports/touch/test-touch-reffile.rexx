/* test-touch-reffile.rexx -- -r flag: copy timestamp from reference file */
/* Usage: rx WORK:test-touch-reffile.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

reffile = 'T:touch_ref_001.txt'
tfile   = 'T:touch_ref_dst_001.txt'

/* Clean up leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || reffile
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Create reference file */
IF OPEN('sf', reffile, 'W') THEN DO
    CALL WRITELN('sf', 'reference file')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create reference file'
    EXIT 10
END

/* Touch target using reference file for timestamp */
ADDRESS COMMAND 'WORK:touch -r ' || reffile || ' ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -r returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

/* Both files must exist */
IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -r did not create target file'
    ADDRESS COMMAND 'Delete >NIL: ' || reffile
    EXIT 10
END
