/* test-rm-recursive.rexx -- remove a directory tree with -R */
/* Usage: rx WORK:test-rm-recursive.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

treedir = 'T:rm_recursive_001'
subdir  = 'T:rm_recursive_001/sub'
file1   = 'T:rm_recursive_001/file1.txt'
file2   = 'T:rm_recursive_001/sub/file2.txt'

ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
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
    CALL WRITELN('f1', 'file in root')
    CALL CLOSE('f1')
END
ELSE DO
    SAY 'FAIL: could not create file1'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF OPEN('f2', file2, 'W') THEN DO
    CALL WRITELN('f2', 'file in subdir')
    CALL CLOSE('f2')
END
ELSE DO
    SAY 'FAIL: could not create file2'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -R ' || treedir
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -R returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF EXISTS(treedir) THEN DO
    SAY 'FAIL: tree directory still exists after rm -R'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

SAY 'OK'
EXIT 0
