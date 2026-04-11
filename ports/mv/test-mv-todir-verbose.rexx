/* test-mv-todir-verbose.rexx -- test mv -v file into directory shows output */
/* Usage: rx WORK:test-mv-todir-verbose.rexx */
/* Prints the mv -v stdout line */
OPTIONS FAILAT 21

srcfile = 'T:mv_todirv_src_001.txt'
dstdir  = 'T:mv_todirv_dir_001'
outfile = 'T:mv_todirv_out_001.txt'

/* Clean up any leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || srcfile
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
ADDRESS COMMAND 'Delete >NIL: ' || outfile

/* Create source file */
IF OPEN('sf', srcfile, 'W') THEN DO
    CALL WRITELN('sf', 'verbose dir move test')
    CALL CLOSE('sf')
END
ELSE DO
    SAY 'FAIL: could not create source file'
    EXIT 10
END

/* Create destination directory */
ADDRESS COMMAND 'MakeDir ' || dstdir
IF RC ~= 0 THEN DO
    SAY 'FAIL: could not create destination directory'
    ADDRESS COMMAND 'Delete >NIL: ' || srcfile
    EXIT 10
END

/* Run mv -v, capture stdout */
ADDRESS COMMAND 'WORK:mv -v ' || srcfile || ' ' || dstdir || ' >' || outfile

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
ADDRESS COMMAND 'Delete >NIL: ' || dstdir || ' ALL'
ADDRESS COMMAND 'Delete >NIL: ' || outfile
ADDRESS COMMAND 'Delete >NIL: ' || srcfile

EXIT 0
