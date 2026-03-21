/* test-runner.rexx — ARexx test harness for FS-UAE automated testing
 *
 * Runs test cases defined in WORK:test-cases.txt, captures output,
 * compares against expected values, and writes TAP-format results
 * to RESULTS:tap-output.txt.
 *
 * TAP (Test Anything Protocol) format:
 *   1..N
 *   ok 1 - description
 *   not ok 2 - description
 *
 * Test case file format (test-cases.txt):
 *   Each test is 3 lines:
 *     TEST: description
 *     CMD: command to run
 *     EXPECT: expected output (single line)
 *
 * Usage: rx WORK:test-runner.rexx
 */

/* Allow commands to return non-zero RC without triggering ARexx ERROR.
 * Amiga return codes: RETURN_OK=0, RETURN_WARN=5, RETURN_ERROR=10, RETURN_FAIL=20.
 * Only truly catastrophic failures (RC >= 21) should trigger ERROR. */
OPTIONS FAILAT 21

/* Read test cases */
testfile = 'WORK:test-cases.txt'
resultfile = 'RESULTS:tap-output.txt'
sentinelfile = 'RESULTS:tests-complete'

IF ~OPEN('tf', testfile, 'R') THEN DO
    SAY 'ERROR: Cannot open' testfile
    ADDRESS COMMAND 'echo "Bail out! Cannot open test cases file" >>' resultfile
    ADDRESS COMMAND 'echo done >' sentinelfile
    EXIT 20
END

/* Parse test cases into simple indexed arrays */
testcount = 0
DO WHILE ~EOF('tf')
    line = READLN('tf')
    IF LEFT(line, 5) = 'TEST:' THEN DO
        testcount = testcount + 1
        desc.testcount = STRIP(SUBSTR(line, 6))
    END
    ELSE IF LEFT(line, 4) = 'CMD:' THEN DO
        cmd.testcount = STRIP(SUBSTR(line, 5))
    END
    ELSE IF LEFT(line, 7) = 'EXPECT:' THEN DO
        expect.testcount = STRIP(SUBSTR(line, 8))
    END
END
CALL CLOSE('tf')

/* Write TAP header */
IF ~OPEN('rf', resultfile, 'W') THEN DO
    SAY 'ERROR: Cannot open' resultfile
    EXIT 20
END
CALL WRITELN('rf', '1..' || testcount)

/* Run each test */
passed = 0
failed = 0
DO i = 1 TO testcount
    tdesc = desc.i
    tcmd = cmd.i
    texpect = expect.i
    outfile = 'T:test_out_' || i || '.txt'

    /* Run the command, redirect output to temp file.
     * Use a shell script via Execute with FailAt 21 to ensure
     * stdout is captured even when commands return non-zero RC
     * (e.g., diff returns RC=5 when files differ). */
    scriptfile = 'T:test_cmd_' || i
    IF OPEN('scr', scriptfile, 'W') THEN DO
        CALL WRITELN('scr', 'FailAt 21')
        CALL WRITELN('scr', tcmd '>' outfile)
        CALL CLOSE('scr')
    END
    ADDRESS COMMAND 'Execute' scriptfile
    cmdrc = RC
    ADDRESS COMMAND 'Delete >NIL:' scriptfile

    /* Read actual output */
    actual = ''
    IF OPEN('of', outfile, 'R') THEN DO
        actual = READLN('of')
        CALL CLOSE('of')
    END

    /* Compare */
    IF STRIP(actual) = STRIP(texpect) THEN DO
        CALL WRITELN('rf', 'ok' i '- ' || tdesc)
        passed = passed + 1
    END
    ELSE DO
        CALL WRITELN('rf', 'not ok' i '- ' || tdesc)
        CALL WRITELN('rf', '#   expected:' texpect)
        CALL WRITELN('rf', '#   actual:  ' actual)
        failed = failed + 1
    END

    /* Clean up temp file */
    ADDRESS COMMAND 'Delete >NIL:' outfile
END

/* Write summary */
CALL WRITELN('rf', '# passed:' passed 'failed:' failed 'total:' testcount)
CALL CLOSE('rf')

/* Write sentinel file to signal completion */
IF OPEN('sf', sentinelfile, 'W') THEN DO
    CALL WRITELN('sf', 'TESTS_COMPLETE')
    CALL WRITELN('sf', 'passed=' || passed)
    CALL WRITELN('sf', 'failed=' || failed)
    CALL WRITELN('sf', 'total=' || testcount)
    CALL CLOSE('sf')
END

SAY 'Tests complete:' passed '/' testcount 'passed'

/* Quit the emulator */
ADDRESS COMMAND 'C:UAEQuit'

EXIT 0
