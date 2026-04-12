/* test-rm-force-existing.rexx -- verify -f removes existing file silently */
/* Usage: rx WORK:test-rm-force-existing.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

testfile = 'T:rm_force_exist_001.txt'

ADDRESS COMMAND 'Delete >NIL: ' || testfile

IF OPEN('sf', testfile, 'W') THEN DO
    CALL WRITELN('sf', 'force existing test')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -f ' || testfile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -f returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

IF EXISTS(testfile) THEN DO
    SAY 'FAIL: file still exists after rm -f'
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

SAY 'OK'
EXIT 0
