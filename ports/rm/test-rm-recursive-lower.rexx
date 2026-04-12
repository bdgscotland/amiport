/* test-rm-recursive-lower.rexx -- verify lowercase -r works same as -R */
/* Usage: rx WORK:test-rm-recursive-lower.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

treedir = 'T:rm_lower_r_001'
file1   = 'T:rm_lower_r_001/item.txt'

ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
ADDRESS COMMAND 'MakeDir ' || treedir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create tree root'
    EXIT 10
END

IF OPEN('f1', file1, 'W') THEN DO
    CALL WRITELN('f1', 'lowercase r test')
    CALL CLOSE('f1')
END
ELSE DO
    SAY 'FAIL: could not create file'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -r ' || treedir
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -r returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

IF EXISTS(treedir) THEN DO
    SAY 'FAIL: directory still exists after rm -r'
    ADDRESS COMMAND 'Delete >NIL: ' || treedir || ' ALL'
    EXIT 10
END

SAY 'OK'
EXIT 0
