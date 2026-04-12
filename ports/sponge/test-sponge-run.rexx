/* test-sponge-run.rexx -- wrapper for sponge FS-UAE tests
 * Usage: rx WORK:test-sponge-run.rexx <mode> [args...]
 *
 * Modes:
 *   basic <inputfile> <outputfile>  -- run sponge, print first line of output
 *   multi <inputfile> <outputfile>  -- run sponge, print all lines of output
 *   overwrite                       -- write file, overwrite with sponge, print result
 *   noarg                           -- run sponge with no args, report RC
 *   twoarg                          -- run sponge with two args, report RC
 *   badflags                        -- run sponge with bad flag, report RC
 *   empty                           -- empty input produces empty output
 *   large                           -- large input fully buffered
 *   newfile <inputfile> <outputfile> -- create new output file
 *   multiline-check                 -- verify multiline input preserved
 *
 * The test harness captures stdout from this script. Sponge writes to a file,
 * so we read the file and print it after running sponge.
 *
 * ARexx ADDRESS COMMAND does not support < stdin redirection in CMD lines.
 * We use an Execute script to perform the < redirect, following the proven
 * pattern from tee and col wrappers.
 */
OPTIONS FAILAT 21

PARSE ARG mode rest
mode = STRIP(mode)

/* Shared temp files for the Execute script pattern */
scriptfile = 'T:sponge_run_cmd.txt'
rcfile     = 'T:sponge_run_rc.txt'

SELECT
    WHEN mode = 'basic' THEN DO
        PARSE VAR rest infile ' ' outfile
        infile = STRIP(infile)
        outfile = STRIP(outfile)
        /* Clean up any previous output file */
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        /* Run sponge via Execute script for proper < redirection */
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        /* Read and print first line of output file */
        IF OPEN('of', outfile, 'R') THEN DO
            line = READLN('of')
            CALL CLOSE('of')
            SAY line
        END
        ELSE DO
            SAY 'ERROR: output file not created'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    WHEN mode = 'multi' THEN DO
        PARSE VAR rest infile ' ' outfile
        infile = STRIP(infile)
        outfile = STRIP(outfile)
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        IF OPEN('of', outfile, 'R') THEN DO
            DO WHILE ~EOF('of')
                line = READLN('of')
                SAY line
            END
            CALL CLOSE('of')
        END
        ELSE DO
            SAY 'ERROR: output file not created'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    WHEN mode = 'overwrite' THEN DO
        outfile = 'T:sponge-overwrite-out.txt'
        infile = 'T:sponge-overwrite-in.txt'
        /* Write initial content to the output file */
        IF OPEN('pf', outfile, 'W') THEN DO
            CALL WRITELN('pf', 'old content')
            CALL CLOSE('pf')
        END
        /* Write new content to input file */
        IF OPEN('inf', infile, 'W') THEN DO
            CALL WRITELN('inf', 'new content')
            CALL CLOSE('inf')
        END
        /* Run sponge via Execute script */
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        /* Read back result */
        IF OPEN('of', outfile, 'R') THEN DO
            line = READLN('of')
            CALL CLOSE('of')
            SAY line
        END
        ELSE DO
            SAY 'ERROR: output file missing after overwrite'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        ADDRESS COMMAND 'Delete >NIL: ' || infile
        EXIT sprc
    END

    WHEN mode = 'empty' THEN DO
        outfile = 'T:sponge-empty-out.txt'
        infile = 'WORK:test-empty.txt'
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        /* File should exist but be empty -- measure its size via attempt to read */
        IF OPEN('of', outfile, 'R') THEN DO
            line = READLN('of')
            CALL CLOSE('of')
            IF line = '' THEN
                SAY 'empty-ok'
            ELSE
                SAY 'ERROR: expected empty, got: ' || line
        END
        ELSE DO
            SAY 'ERROR: output file not created for empty input'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    WHEN mode = 'noarg' THEN DO
        ADDRESS COMMAND 'WORK:sponge >NIL:'
        SAY 'RC=' || RC
        EXIT 0
    END

    WHEN mode = 'twoarg' THEN DO
        ADDRESS COMMAND 'WORK:sponge T:f1.txt T:f2.txt >NIL:'
        SAY 'RC=' || RC
        EXIT 0
    END

    WHEN mode = 'badflags' THEN DO
        ADDRESS COMMAND 'WORK:sponge -z T:out.txt >NIL:'
        SAY 'RC=' || RC
        EXIT 0
    END

    WHEN mode = 'large' THEN DO
        outfile = 'T:sponge-large-out.txt'
        infile = 'WORK:test-sponge-large.txt'
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        /* Print first and last lines to verify */
        IF OPEN('of', outfile, 'R') THEN DO
            firstline = READLN('of')
            lastline = ''
            DO WHILE ~EOF('of')
                line = READLN('of')
                IF line ~= '' THEN lastline = line
            END
            CALL CLOSE('of')
            SAY firstline
            SAY lastline
        END
        ELSE DO
            SAY 'ERROR: large output file not created'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    WHEN mode = 'newfile' THEN DO
        PARSE VAR rest infile ' ' outfile
        infile = STRIP(infile)
        outfile = STRIP(outfile)
        /* Ensure output file does NOT exist beforehand */
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        IF OPEN('of', outfile, 'R') THEN DO
            line = READLN('of')
            CALL CLOSE('of')
            SAY line
        END
        ELSE DO
            SAY 'ERROR: new output file not created'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    WHEN mode = 'multiline-check' THEN DO
        outfile = 'T:sponge-mc-out.txt'
        infile = 'WORK:test-multiline.txt'
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        CALL run_sponge_with_stdin(outfile, infile)
        sprc = RESULT
        IF OPEN('of', outfile, 'R') THEN DO
            /* Read all lines, count them */
            count = 0
            lastline = ''
            DO WHILE ~EOF('of')
                line = READLN('of')
                count = count + 1
                lastline = line
            END
            CALL CLOSE('of')
            SAY count
            SAY lastline
        END
        ELSE DO
            SAY 'ERROR: multiline output file not created'
        END
        ADDRESS COMMAND 'Delete >NIL: ' || outfile
        EXIT sprc
    END

    OTHERWISE DO
        SAY 'ERROR: unknown mode: ' || mode
        EXIT 10
    END
END

EXIT 0

/* ------------------------------------------------------------------ */
/* run_sponge_with_stdin: Run sponge with stdin redirected from a file
 *
 * ARexx ADDRESS COMMAND does not support < stdin redirection.
 * We write an Execute script containing the command with < redirect,
 * then run it via ADDRESS COMMAND 'Execute scriptfile'.
 * This is the same proven pattern used by tee and col wrappers.
 *
 * Arguments:
 *   sponge_outfile -- the output file argument passed to sponge
 *   sponge_infile  -- the file to redirect as stdin via <
 *
 * Returns the sponge return code via RESULT.
 */
run_sponge_with_stdin: PROCEDURE EXPOSE scriptfile rcfile
    PARSE ARG sponge_outfile, sponge_infile

    sponge_outfile = STRIP(sponge_outfile)
    sponge_infile = STRIP(sponge_infile)

    /* Write an Execute script with < stdin redirect */
    IF ~OPEN('sf', scriptfile, 'W') THEN DO
        SAY 'test-sponge-run: cannot write script file'
        RETURN 20
    END
    CALL WRITELN('sf', 'FailAt 21')
    CALL WRITELN('sf', 'WORK:sponge' sponge_outfile '<' sponge_infile)
    CALL WRITELN('sf', 'Echo >' || rcfile '$RC')
    CALL CLOSE('sf')

    /* Run the script */
    ADDRESS COMMAND 'Execute' scriptfile

    /* Read return code */
    cmdrc = 0
    IF OPEN('rcf', rcfile, 'R') THEN DO
        rcline = READLN('rcf')
        CALL CLOSE('rcf')
        IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
    END

    /* Clean up temp files */
    ADDRESS COMMAND 'Delete >NIL:' scriptfile
    ADDRESS COMMAND 'Delete >NIL:' rcfile

    RETURN cmdrc
