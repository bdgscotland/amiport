/* test-rm-dir-d.rexx -- remove an empty directory using -d flag */
/* Usage: rx WORK:test-rm-dir-d.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

testdir = 'T:rm_dir_d_001'

ADDRESS COMMAND 'Delete >NIL: ' || testdir || ' ALL'
ADDRESS COMMAND 'MakeDir ' || testdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create test directory'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -d ' || testdir
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -d returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || testdir || ' ALL'
    EXIT 10
END

IF EXISTS(testdir) THEN DO
    SAY 'FAIL: directory still exists after rm -d'
    ADDRESS COMMAND 'Delete >NIL: ' || testdir || ' ALL'
    EXIT 10
END

SAY 'OK'
EXIT 0
