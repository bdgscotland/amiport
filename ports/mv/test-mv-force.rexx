/* test-mv-force.rexx -- test mv -f force overwrite of existing file */
/* Usage: rx WORK:test-mv-force.rexx */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

srcfile = 'T:mv_force_src_001.txt'
dstfile = 'T:mv_force_dst_001.txt'
outfile = 'T:mv_force_out_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile
ADDRESS COMMAND 'Delete >NIL: ' || outfile

/* Create source file with known content */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'new content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Create existing destination file */
IF OPEN('df', dstfile, 'W') THEN DO
    CALL WRITELN('df', 'old content')
    CALL CLOSE('df')
END
ELSE DO
    SAY 'FAIL: could not create destination file'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END

/* Run mv -f -v: force overwrite, check verbose output */
ADDRESS COMMAND 'WORK:mv -f -v ' || srcfile || ' ' || dstfile || ' >' || outfile

/* Read and print the verbose output line */
IF OPEN('of', outfile, 'R') THEN DO
    line = READLN('of')
    CALL CLOSE('of')
    SAY line
END
ELSE DO
    SAY 'FAIL: no output captured'
END

/* Cleanup */
ADDRESS COMMAND 'Delete >NIL: ' || dstfile
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || srcfile

EXIT 0
