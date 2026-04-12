/* test-touch-create.rexx -- create new file: touch nonexistent, verify it exists */
/* Usage: rx WORK:test-touch-create.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

newfile = 'T:touch_create_001.txt'

/* Clean up any leftover from previous run */
ADDRESS COMMAND 'Delete >NIL: ' || newfile

/* File must not exist yet */
IF EXISTS(newfile) THEN DO
    SAY 'FAIL: file already exists before touch'
    EXIT 10
END

/* Run touch to create the file */
ADDRESS COMMAND 'WORK:touch ' || newfile

/* Verify file was created */
IF EXISTS(newfile) THEN DO
    SAY 'OK'
    ADDRESS COMMAND 'Delete >NIL: ' || newfile
    EXIT 0
END
ELSE DO
    SAY 'FAIL: touch did not create the file'
    EXIT 10
END
