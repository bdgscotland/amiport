/* test-rm-deep-tree.rexx -- create a deeply nested tree (10 levels) and remove */
/* Usage: rx WORK:test-rm-deep-tree.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* Stress tests fts recursion depth on AmigaOS */
OPTIONS FAILAT 21

root = 'T:rm_deep_001'
ADDRESS COMMAND 'Delete >NIL: ' || root || ' ALL'

/* Build path incrementally: T:rm_deep_001/l1/l2/.../l10 */
curdir = root
ADDRESS COMMAND 'MakeDir ' || curdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create root directory'
    EXIT 10
END

DO i = 1 TO 10
    curdir = curdir || '/l' || i
    ADDRESS COMMAND 'MakeDir ' || curdir
    IF RC ~= 0 THEN DO
        SAY 'FAIL: could not create level ' || i || ' directory'
        ADDRESS COMMAND 'Delete >NIL: ' || root || ' ALL'
        EXIT 10
    END
END

/* Create a leaf file */
leaffile = curdir || '/leaf.txt'
IF OPEN('lf', leaffile, 'W') THEN DO
    CALL WRITELN('lf', 'deep leaf file')
    CALL CLOSE('lf')
END
ELSE DO
    SAY 'FAIL: could not create leaf file'
    ADDRESS COMMAND 'Delete >NIL: ' || root || ' ALL'
    EXIT 10
END

ADDRESS COMMAND 'WORK:rm -R ' || root
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -R deep tree returned RC=' || rmrc
    ADDRESS COMMAND 'Delete >NIL: ' || root || ' ALL'
    EXIT 10
END

IF EXISTS(root) THEN DO
    SAY 'FAIL: root still exists after rm -R deep tree'
    ADDRESS COMMAND 'Delete >NIL: ' || root || ' ALL'
    EXIT 10
END

SAY 'OK'
EXIT 0
