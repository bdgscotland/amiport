/* test-vim-edit-wrap.rexx -- run vim ex-mode edit, verify T: output file */
/* Tests vim scripted substitution: opens hello file, subs Hello->Greetings, */
/* writes to T:vim-test-output.txt, quits. Prints first line so harness can  */
/* verify the file was written with the expected content.                     */
OPTIONS FAILAT 21

outfile = 'T:vim-test-output.txt'

/* Clean up any previous run */
ADDRESS COMMAND 'Delete >NIL:' outfile

/* Run vim with -S script: substitute, write to T:, quit */
ADDRESS COMMAND 'WORK:vim -u NONE -S WORK:test-vim-sub.txt WORK:test-vim-hello.txt'
vimrc = RC

IF vimrc ~= 0 THEN DO
    SAY 'ERROR: vim exited with RC=' || vimrc
    EXIT 10
END

/* Read the output file vim wrote to T: */
IF OPEN('of', outfile, 'R') THEN DO
    line = READLN('of')
    CALL CLOSE('of')
    SAY line
END
ELSE DO
    SAY 'ERROR: output file not created'
    EXIT 10
END

/* Cleanup */
ADDRESS COMMAND 'Delete >NIL:' outfile
EXIT 0
