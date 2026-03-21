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

/* Parse test cases */
tests. = ''
testcount = 0
DO WHILE ~EOF('tf')
    line = READLN('tf')
    IF LEFT(line, 5) = 'TEST:' THEN DO
        testcount = testcount + 1
        tests.testcount.desc = STRIP(SUBSTR(line, 6))
    END
    ELSE IF LEFT(line, 4) = 'CMD:' THEN DO
        tests.testcount.cmd = STRIP(SUBSTR(line, 5))
    END
    ELSE IF LEFT(line, 7) = 'EXPECT:' THEN DO
        tests.testcount.expect = STRIP(SUBSTR(line, 8))
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
    desc = tests.i.desc
    cmd = tests.i.cmd
    expect = tests.i.expect
    outfile = 'T:test_out_' || i || '.txt'

    /* Run the command, redirect output to temp file */
    ADDRESS COMMAND cmd '>' outfile

    /* Read actual output */
    actual = ''
    IF OPEN('of', outfile, 'R') THEN DO
        actual = READLN('of')
        CALL CLOSE('of')
    END

    /* Compare */
    IF STRIP(actual) = STRIP(expect) THEN DO
        CALL WRITELN('rf', 'ok' i '-' desc)
        passed = passed + 1
    END
    ELSE DO
        CALL WRITELN('rf', 'not ok' i '-' desc)
        CALL WRITELN('rf', '#   expected:' expect)
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
