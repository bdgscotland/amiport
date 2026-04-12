/* test-touch-nocreate.rexx -- -c flag: no creation of nonexistent file, RC=0 */
/* Usage: rx WORK:test-touch-nocreate.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

nofile = 'T:touch_nocreate_001.txt'

/* Ensure the file does not exist */
ADDRESS COMMAND 'Delete >NIL: ' || nofile

/* Touch -c: should NOT create the file */
ADDRESS COMMAND 'WORK:touch -c ' || nofile
trc = RC

/* RC must be 0 (not an error), but file must not exist */
IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -c returned RC=' || trc || ' (expected 0)'
    EXIT 10
END

IF EXISTS(nofile) THEN DO
    SAY 'FAIL: touch -c created the file (must not create)'
    ADDRESS COMMAND 'Delete >NIL: ' || nofile
    EXIT 10
END
ELSE DO
    SAY 'OK'
    EXIT 0
END
