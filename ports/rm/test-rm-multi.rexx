/* test-rm-multi.rexx -- create two files and remove both at once */
/* Usage: rx WORK:test-rm-multi.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

file1 = 'T:rm_multi_001.txt'
file2 = 'T:rm_multi_002.txt'

ADDRESS COMMAND 'Delete >NIL: ' || file1
ADDRESS COMMAND 'Delete >NIL: ' || file2

IF OPEN('f1', file1, 'W') THEN DO
    CALL WRITELN('f1', 'multi file 1')
    CALL CLOSE('f1')
END
ELSE DO
    SAY 'FAIL: could not create file1'
    EXIT 10
END

IF OPEN('f2', file2, 'W') THEN DO
    CALL WRITELN('f2', 'multi file 2')
    CALL CLOSE('f2')
END
ELSE DO
    SAY 'FAIL: could not create file2'
    ADDRESS COMMAND 'Delete >NIL: ' || file1
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm ' || file1 || ' ' || file2
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || file1
    ADDRESS COMMAND 'Delete >NIL: ' || file2
    EXIT 10
END

IF EXISTS(file1) THEN DO
    SAY 'FAIL: file1 still exists'
    ADDRESS COMMAND 'Delete >NIL: ' || file1
    ADDRESS COMMAND 'Delete >NIL: ' || file2
    EXIT 10
END

IF EXISTS(file2) THEN DO
    SAY 'FAIL: file2 still exists'
    ADDRESS COMMAND 'Delete >NIL: ' || file2
    EXIT 10
END

SAY 'OK'
EXIT 0
