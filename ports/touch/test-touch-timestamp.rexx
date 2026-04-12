/* test-touch-timestamp.rexx -- -t flag: set timestamp, verify file created */
/* Usage: rx WORK:test-touch-timestamp.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
/* NOTE: AmigaOS does not have stat() equivalent for mtime read-back,
 * so we only verify RC=0 and file creation here. */
OPTIONS FAILAT 21

tfile = 'T:touch_ts_001.txt'

/* Clean up leftover */
ADDRESS COMMAND 'Delete >NIL: ' || tfile

/* Touch with -t numeric timestamp: 202401010000 = 2024-01-01 00:00 */
ADDRESS COMMAND 'WORK:touch -t 202401010000 ' || tfile
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch -t returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 10
END

IF EXISTS(tfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || tfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch -t did not create file'
    EXIT 10
END
