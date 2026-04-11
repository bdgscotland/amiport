/* test-mv-longname.rexx -- test mv with long filename (near PATH_MAX) */
/* Usage: rx WORK:test-mv-longname.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* AmigaDOS supports filenames up to 107 characters (OFS/FFS limit) */
/* This tests filenames at a practical long length */
OPTIONS FAILAT 21

/* 60-character filename (safe for all AmigaDOS filesystems) */
srcname = 'mv_longname_test_sixty_chars_abcdefghijklmnopqrstuv_001.txt'
dstname = 'mv_longname_dest_sixty_chars_abcdefghijklmnopqrstuv_001.txt'
srcfile = 'T:' || srcname
dstfile = 'T:' || dstname

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile

/* Create source file with long name */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'long filename test content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create long-name source file'
    EXIT 10
END

/* Run mv */
ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstfile

/* Verify result */
IF EXISTS(dstfile) THEN DO
    IF ~EXISTS(srcfile) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: source still exists after long-name mv'
        ADDRESS COMMAND 'Delete >NIL: ' || srcfile
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: destination does not exist for long-name mv'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END
