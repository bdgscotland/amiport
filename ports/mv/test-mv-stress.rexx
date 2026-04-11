/* test-mv-stress.rexx -- stress test: move 50 files into a directory */
/* Usage: rx WORK:test-mv-stress.rexx */
/* Prints "OK N/N" on success, "FAIL" on failure */
OPTIONS FAILAT 21

dstdir = 'T:mv_stress_dir_001'
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
ADDRESS COMMAND 'MakeDir ' || dstdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create destination directory'
    EXIT 10
END

/* Create 50 temp files and move them in batches */
failed = 0
total = 0

DO i = 1 TO 50
    srcfile = 'T:mv_stress_src_' || i || '_001.txt'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    IF OPEN('sf', srcfile, 'W') THEN DO
        CALL WRITELN('sf', 'stress file ' || i)
        CALL CLOSE('sf')
    END
    /* Move this file */
    ADDRESS COMMAND 'WORK:mv ' || srcfile || ' ' || dstdir
    total = total + 1
    /* Check it arrived */
    dstfile = dstdir || '/mv_stress_src_' || i || '_001.txt'
    IF ~EXISTS(dstfile) THEN failed = failed + 1
END

/* Cleanup */
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'

IF failed = 0 THEN DO
    SAY 'OK ' || total || '/' || total
    EXIT 0
END
ELSE DO
    SAY 'FAIL: ' || failed || ' files not moved'
    EXIT 10
END
