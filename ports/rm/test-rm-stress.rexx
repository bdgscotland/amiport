/* test-rm-stress.rexx -- create 20 files and remove them all at once */
/* Usage: rx WORK:test-rm-stress.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

prefix = 'T:rm_stress_'
count = 20

/* Clean up any leftovers */
DO i = 1 TO count
    ADDRESS COMMAND 'Delete >NIL: ' || prefix || i || '.txt'
END

/* Create all files */
DO i = 1 TO count
    fname = prefix || i || '.txt'
    IF OPEN('sf', fname, 'W') THEN DO
        CALL WRITELN('sf', 'stress test line ' || i)
        CALL CLOSE('sf')
    END
    ELSE DO
        SAY 'FAIL: could not create file ' || i
        DO j = 1 TO i
            ADDRESS COMMAND 'Delete >NIL: ' || prefix || j || '.txt'
        END
        EXIT 10
    END
END

/* Build argument list and remove all at once */
arglist = ''
DO i = 1 TO count
    arglist = arglist || ' ' || prefix || i || '.txt'
END

ADDRESS COMMAND 'WORK:rm' || arglist
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm stress returned RC=' || rmrc
    DO i = 1 TO count
        ADDRESS COMMAND 'Delete >NIL: ' || prefix || i || '.txt'
    END
    EXIT 10
END

/* Verify all are gone */
DO i = 1 TO count
    IF EXISTS(prefix || i || '.txt') THEN DO
        SAY 'FAIL: file ' || i || ' still exists'
        DO j = i TO count
            ADDRESS COMMAND 'Delete >NIL: ' || prefix || j || '.txt'
        END
        EXIT 10
    END
END

SAY 'OK'
EXIT 0
