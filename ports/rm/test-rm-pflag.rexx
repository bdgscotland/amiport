/* test-rm-pflag.rexx -- verify -P flag is accepted and removes the file */
/* Usage: rx WORK:test-rm-pflag.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Note: -P (secure overwrite) is a no-op on AmigaOS but the file is still removed */
OPTIONS FAILAT 21

testfile = 'T:rm_pflag_001.txt'

ADDRESS COMMAND 'Delete >NIL: ' || testfile

IF OPEN('sf', testfile, 'W') THEN DO
    CALL WRITELN('sf', 'pflag test content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -P ' || testfile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -P returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

IF EXISTS(testfile) THEN DO
    SAY 'FAIL: file still exists after rm -P'
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    EXIT 10
END

SAY 'OK'
EXIT 0
