/* test-rm-single.rexx -- create a file in T: and remove it */
/* Usage: rx WORK:test-rm-single.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

testfile = 'T:rm_single_001.txt'

ADDRESS COMMAND 'Delete >NIL: ' || testfile

IF OPEN('sf', testfile, 'W') THEN DO
    CALL WRITELN('sf', 'single file test')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm ' || testfile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

IF EXISTS(testfile) THEN DO
    SAY 'FAIL: file still exists after rm'
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

SAY 'OK'
EXIT 0
