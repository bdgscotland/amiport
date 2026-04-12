/* test-rm-force-nonexist.rexx -- verify -f suppresses error for missing file */
/* Usage: rx WORK:test-rm-force-nonexist.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

nofile = 'T:rm_force_noexist_001.txt'

ADDRESS COMMAND 'Delete >NIL: ' || nofile

ADDRESS COMMAND 'WORK:rm -f ' || nofile
rmrc = RC

IF rmrc ~= 0 THEN DO
    SAY 'FAIL: rm -f on nonexistent returned RC=' || rmrc || ' (expected 0)'
    EXIT 10
END

SAY 'OK'
EXIT 0
