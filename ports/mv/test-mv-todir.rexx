/* test-mv-todir.rexx -- test mv file into directory */
/* Usage: rx WORK:test-mv-todir.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

srcfile = 'T:mv_todir_src_001.txt'
dstdir  = 'T:mv_todir_dir_001'
dstfile = 'T:mv_todir_dir_001/mv_todir_src_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'

/* Create source file */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'move to directory test')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Create destination directory */
ADDRESS COMMAND 'MakeDir ' || dstdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create destination directory'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END

/* Run mv */
ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstdir

/* Verify: dst file exists inside dir, src gone */
IF EXISTS(dstfile) THEN DO
    IF ~EXISTS(srcfile) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: source still exists after mv to dir'
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile
        ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: destination file does not exist in dir'
    ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
    EXIT 10
END
