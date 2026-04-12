/* test-touch-multifile.rexx -- touch multiple files at once */
/* Usage: rx WORK:test-touch-multifile.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

f1 = 'T:touch_multi_001.txt'
f2 = 'T:touch_multi_002.txt'
f3 = 'T:touch_multi_003.txt'

/* Clean up leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || f1
ADDRESS COMMAND 'Delete >NIL: ' || f2
ADDRESS COMMAND 'Delete >NIL: ' || f3

/* Touch three files at once */
ADDRESS COMMAND 'WORK:touch ' || f1 || ' ' || f2 || ' ' || f3
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || f1
    ADDRESS COMMAND 'Delete >NIL: ' || f2
    ADDRESS COMMAND 'Delete >NIL: ' || f3
    EXIT 10
END

/* All three must exist */
IF ~EXISTS(f1) THEN DO
    SAY 'FAIL: file 1 not created'
    ADDRESS COMMAND 'Delete >NIL: ' || f2
    ADDRESS COMMAND 'Delete >NIL: ' || f3
    EXIT 10
END
IF ~EXISTS(f2) THEN DO
    SAY 'FAIL: file 2 not created'
    ADDRESS COMMAND 'Delete >NIL: ' || f1
    ADDRESS COMMAND 'Delete >NIL: ' || f3
    EXIT 10
END
IF ~EXISTS(f3) THEN DO
    SAY 'FAIL: file 3 not created'
    ADDRESS COMMAND 'Delete >NIL: ' || f1
    ADDRESS COMMAND 'Delete >NIL: ' || f2
    EXIT 10
END

SAY 'OK'
ADDRESS COMMAND 'Delete >NIL: ' || f1
ADDRESS COMMAND 'Delete >NIL: ' || f2
ADDRESS COMMAND 'Delete >NIL: ' || f3
EXIT 0
