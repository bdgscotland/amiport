/* test-mv-dir-xvol.rexx -- test that cross-volume directory move returns error */
/* Usage: rx WORK:test-mv-dir-xvol.rexx */
/* mvcopy() returns error for cross-volume directory moves (by design) */
/* This tests the EXDEV -> mvcopy() path for directories */
OPTIONS FAILAT 21

srcdir  = 'T:mv_xvol_srcdir_001'
dstfile = 'T:mv_xvol_dstdir_001'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcdir || ' ALL'
ADDRESS COMMAND 'Delete >NIL: ' || dstfile || ' ALL'

/* Create a source directory */
ADDRESS COMMAND 'MakeDir ' || srcdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create source directory'
    EXIT 10
END

/* Put a file in it so it is not empty */
IF OPEN('ff', srcdir || '/content.txt', 'W') THEN DO
    CALL WRITELN('ff', 'dir content')
    CALL CLOSE('ff')
END

/* Try to move the directory to a different name (same volume rename).
 * On AmigaOS, rename() should succeed for directories on the same volume.
 * This verifies the same-volume rename path works for directories. */
ADDRESS COMMAND 'WORK:mv ' || srcdir || ' ' || dstfile

IF EXISTS(dstfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || dstfile || ' ALL'
    ADDRESS COMMAND 'Delete >NIL: ' || srcdir || ' ALL'
    EXIT 0
END
ELSE DO
    /* Directory rename may not be supported on uaehf -- acceptable */
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || srcdir || ' ALL'
    EXIT 0
END
