/* test-rm-force-recursive.rexx -- verify -fR: force-recursive on existing tree */
/* Usage: rx WORK:test-rm-force-recursive.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Simulates the common cleanup idiom: rm -fR dirname */
OPTIONS FAILAT 21

treedir  = 'T:rm_frec_001'
subdir   = 'T:rm_frec_001/sub'
file1    = 'T:rm_frec_001/file.txt'
file2    = 'T:rm_frec_001/sub/nested.txt'
noexist  = 'T:rm_frec_no_such_dir'

ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
ADDRESS COMMAND 'Delete >NIL: ' || noexist || ' ALL'

ADDRESS COMMAND 'MakeDir ' || treedir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create tree root'
    EXIT 10
END
ADDRESS COMMAND 'MakeDir ' || subdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create subdirectory'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF OPEN('f1', file1, 'W') THEN DO
    CALL WRITELN('f1', 'force recursive test')
    CALL CLOSE('f1')
END
ELSE DO
    SAY 'FAIL: could not create file1'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF OPEN('f2', file2, 'W') THEN DO
    CALL WRITELN('f2', 'nested file')
    CALL CLOSE('f2')
END
ELSE DO
    SAY 'FAIL: could not create file2'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

/* -fR on existing tree plus a nonexistent path: both should succeed silently */
ADDRESS COMMAND 'WORK:rm -fR ' || treedir || ' ' || noexist
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -fR returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF EXISTS(treedir) THEN DO
    SAY 'FAIL: tree still exists after rm -fR'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

SAY 'OK'
EXIT 0
