/* test-touch-stress.rexx -- stress: create 20 files with single touch invocation */
/* Usage: rx WORK:test-touch-stress.rexx */
/* Prints "OK N/20 files created" or "FAIL: reason" */
/* Tests memory pool and argument handling under realistic load */
OPTIONS FAILAT 21

/* Build list of 20 unique temp file paths */
base = 'T:touch_stress_'
files = ''
DO i = 1 TO 20
    fname = base || i || '.txt'
    /* Clean up leftover */
    ADDRESS COMMAND 'Delete >NIL: ' || fname
    IF i = 1 THEN
        files = fname
    ELSE
        files = files || ' ' || fname
END

/* Touch all 20 files in one invocation */
ADDRESS COMMAND 'WORK:touch ' || files
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch returned RC=' || trc
    DO i = 1 TO 20
        ADDRESS COMMAND 'Delete >NIL: ' || base || i || '.txt'
    END
    EXIT 10
END

/* Count how many were created */
created = 0
DO i = 1 TO 20
    fname = base || i || '.txt'
    IF EXISTS(fname) THEN
        created = created + 1
    ADDRESS COMMAND 'Delete >NIL: ' || fname
END

IF created = 20 THEN DO
    SAY 'OK'
    EXIT 0
END
ELSE DO
    SAY 'FAIL: only ' || created || '/20 files created'
    EXIT 10
END
