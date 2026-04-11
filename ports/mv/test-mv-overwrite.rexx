/* test-mv-overwrite.rexx -- test mv overwrites existing file without -f */
/* Usage: rx WORK:test-mv-overwrite.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

srcfile = 'T:mv_overwrite_src_001.txt'
dstfile = 'T:mv_overwrite_dst_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile

/* Create source file with known content */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'source data')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Create existing destination file */
IF OPEN('df', dstfile, 'W') THEN DO
    CALL WRITELN('df', 'existing data')
    CALL CLOSE('df')
END
ELSE DO
    SAY 'FAIL: could not create destination file'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END

/* Run mv without -f -- should overwrite silently */
ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstfile

/* Verify: dst exists, src gone */
IF EXISTS(dstfile) THEN DO
    IF ~EXISTS(srcfile) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: source still exists after overwrite'
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: destination missing after overwrite'
    EXIT 10
END
