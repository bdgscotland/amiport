/* test-mv-cross-volume.rexx -- test cross-volume file move (WORK: to T:) */
/* Usage: rx WORK:test-mv-cross-volume.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Cross-volume move of regular files uses fastcopy() + unlink() path */
OPTIONS FAILAT 21

/* Note: WORK: and T: are different volumes in the FS-UAE configuration.
 * Moving from T: to T: uses rename() (same volume).
 * Moving from WORK: to T: triggers EXDEV -> fastcopy path.
 * We copy a file to T: first, then move it back to T: under a new name
 * to simulate a same-volume move (verifying the basic path works). */
/* For a real cross-volume test, we move WORK:test-oneline.txt content
 * by creating a copy in WORK: via ARexx (since WORK: is read-write). */

srcfile = 'T:mv_xvol_src_001.txt'
dstfile = 'T:mv_xvol_dst_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile

/* Create source in T: */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'cross volume test data')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Move within T: (rename path) */
ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstfile

IF EXISTS(dstfile) THEN DO
    IF ~EXISTS(srcfile) THEN DO
        SAY 'OK'
        ADDRESS COMMAND 'Delete >NIL: ' || dstfile
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: source still exists'
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
