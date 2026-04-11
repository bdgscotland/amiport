/* test-mv-multi.rexx -- test mv multiple files into a directory */
/* Usage: rx WORK:test-mv-multi.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

srcfile1 = 'T:mv_multi_src1_001.txt'
srcfile2 = 'T:mv_multi_src2_001.txt'
srcfile3 = 'T:mv_multi_src3_001.txt'
dstdir   = 'T:mv_multi_dir_001'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile1
ADDRESS COMMAND 'Delete >NIL: ' || srcfile2
ADDRESS COMMAND 'Delete >NIL: ' || srcfile3
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'

/* Create source files */
IF OPEN('f1', srcfile1, 'W') THEN DO
    CALL WRITELN('f1', 'file one')
    CALL CLOSE('f1')
END
IF OPEN('f2', srcfile2, 'W') THEN DO
    CALL WRITELN('f2', 'file two')
    CALL CLOSE('f2')
END
IF OPEN('f3', srcfile3, 'W') THEN DO
    CALL WRITELN('f3', 'file three')
    CALL CLOSE('f3')
END

/* Create destination directory */
ADDRESS COMMAND 'MakeDir ' || dstdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create destination directory'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile1
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile2
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile3
    EXIT 10
END

/* Run mv with multiple sources */
ADDRESS COMMAND 'WORK:mv ' || srcfile1 || ' ' || srcfile2 || ' ' || srcfile3 || ' ' || dstdir

/* Verify all three files moved */
dst1 = dstdir || '/mv_multi_src1_001.txt'
dst2 = dstdir || '/mv_multi_src2_001.txt'
dst3 = dstdir || '/mv_multi_src3_001.txt'

IF EXISTS(dst1) & EXISTS(dst2) & EXISTS(dst3) THEN DO
    IF ~EXISTS(srcfile1) & ~EXISTS(srcfile2) & ~EXISTS(srcfile3) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: some source files still exist'
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile1
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile2
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile3
        ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: not all destination files exist'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile1
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile2
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile3
    ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
    EXIT 10
END
