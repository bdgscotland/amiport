/* test-mv-basic.rexx -- basic rename: create src, mv to dst, verify */
/* Usage: rx WORK:test-mv-basic.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

srcfile = 'T:mv_basic_src_001.txt'
dstfile = 'T:mv_basic_dst_001.txt'

/* Clean up any leftovers from previous run */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile

/* Create source file */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'basic rename test')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Run mv */
ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstfile

/* Verify: dst exists, src gone */
IF EXISTS(dstfile) THEN DO
    IF ~EXISTS(srcfile) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: source still exists after mv'
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: destination does not exist'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END
