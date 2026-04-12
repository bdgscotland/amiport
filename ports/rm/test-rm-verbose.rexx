/* test-rm-verbose.rexx -- verify -v prints filename to stdout */
/* Usage: rx WORK:test-rm-verbose.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

testfile = 'T:rm_verbose_001.txt'
outfile  = 'T:rm_verbose_out.txt'

ADDRESS COMMAND 'Delete >NIL: ' || testfile
ADDRESS COMMAND 'Delete >NIL: ' || outfile

IF OPEN('sf', testfile, 'W') THEN DO
    CALL WRITELN('sf', 'verbose test content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create test file'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -v ' || testfile || ' >' || outfile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -v returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END

IF EXISTS(testfile) THEN DO
    SAY 'FAIL: file still exists after rm -v'
    ADDRESS COMMAND 'Delete >NIL: ' || testfile
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END

IF ~OPEN('of', outfile, 'R') THEN DO
    SAY 'FAIL: could not open output file'
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END
line = READLN('of')
CALL CLOSE('of')
ADDRESS COMMAND 'Delete >NIL: ' || outfile

IF line = testfile THEN DO
    SAY 'OK'
    EXIT 0
END
ELSE DO
    SAY 'FAIL: expected "' || testfile || '" got "' || line || '"'
    EXIT 10
END
