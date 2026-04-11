/* test-mv-verbose.rexx -- test mv -v verbose output */
/* Usage: rx WORK:test-mv-verbose.rexx */
/* Prints the mv -v stdout line ("src -> dst") */
OPTIONS FAILAT 21

srcfile = 'T:mv_verbose_src_001.txt'
dstfile = 'T:mv_verbose_dst_001.txt'
outfile = 'T:mv_verbose_out_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstfile
ADDRESS COMMAND 'Delete >NIL: ' || outfile

/* Create source file */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'verbose test content')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Run mv -v, capture stdout */
ADDRESS COMMAND 'WORK:mv -v ' || srcfile || ' ' || dstfile || ' >' || outfile

/* Read and print the output line */
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
