/* test-rm-verbose-recursive.rexx -- verify -Rv prints all removed paths */
/* Usage: rx WORK:test-rm-verbose-recursive.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

treedir = 'T:rm_vrec_001'
file1   = 'T:rm_vrec_001/alpha.txt'
outfile = 'T:rm_vrec_out.txt'

ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'MakeDir ' || treedir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create tree root'
    EXIT 10
END

IF OPEN('f1', file1, 'W') THEN DO
    CALL WRITELN('f1', 'verbose recursive test')
    CALL CLOSE('f1')
END
ELSE DO
    SAY 'FAIL: could not create file'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -Rv ' || treedir || ' >' || outfile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -Rv returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END

IF EXISTS(treedir) THEN DO
    SAY 'FAIL: directory still exists after rm -Rv'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END

IF ~OPEN('of', outfile, 'R') THEN DO
    SAY 'FAIL: could not open output file'
    ADDRESS COMMAND 'Delete >NIL: ' || outfile
    EXIT 10
END
/* Output should contain the file path and the dir path (2 lines) */
line1 = READLN('of')
line2 = READLN('of')
CALL CLOSE('of')
ADDRESS COMMAND 'Delete >NIL: ' || outfile

/* Check that file1 path appears on first line */
IF line1 = file1 THEN DO
    /* Check that treedir path appears on second line */
    IF line2 = treedir THEN DO
        SAY 'OK'
        EXIT 0
    END
    ELSE DO
        SAY 'FAIL: line2 expected "' || treedir || '" got "' || line2 || '"'
        EXIT 10
    END
END
ELSE DO
    SAY 'FAIL: line1 expected "' || file1 || '" got "' || line1 || '"'
    EXIT 10
END
