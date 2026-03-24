/* test-runner.rexx -- ARexx test harness for FS-UAE automated testing
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
 *   Each test is a block starting with TEST: and containing:
 *     TEST: description
 *     CMD: command to run
 *     EXPECT: expected output (exact first-line match)
 *   Optional assertion lines (can combine with EXPECT:):
 *     EXPECT_CONTAINS: substring to find in output (instead of EXPECT:)
 *     EXPECT_RC: expected Amiga return code (0, 5, 10, or 20)
 *
 *   Interactive tests (ADR-023) use ITEST: blocks:
 *     ITEST: description
 *     LAUNCH: command to run in background
 *     KEYS: comma-separated key sequence for KeyInject
 *     EXPECT_RC: expected return code (optional, default 0)
 *   KeyInject must be at WORK:KeyInject (deployed by test-fsemu.sh).
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

/* Parse test cases into simple indexed arrays.
 * Uses SELECT/WHEN for robust parsing and strips CR characters
 * to handle macOS line endings on AmigaOS. */
testcount = 0
DO WHILE ~EOF('tf')
    line = READLN('tf')
    line = STRIP(line, 'T', '0D'x)  /* Strip CR if present (macOS -> AmigaOS) */
    SELECT
        WHEN LEFT(line, 6) = 'ITEST:' THEN DO
            testcount = testcount + 1
            desc.testcount = STRIP(SUBSTR(line, 7))
            interactive.testcount = 1
            expect_mode.testcount = 'EXACT'
        END
        WHEN LEFT(line, 7) = 'LAUNCH:' THEN DO
            launch.testcount = STRIP(SUBSTR(line, 8))
        END
        WHEN LEFT(line, 5) = 'KEYS:' THEN DO
            keys.testcount = STRIP(SUBSTR(line, 6))
        END
        WHEN LEFT(line, 5) = 'TEST:' THEN DO
            testcount = testcount + 1
            desc.testcount = STRIP(SUBSTR(line, 6))
            expect_mode.testcount = 'EXACT'
        END
        WHEN LEFT(line, 4) = 'CMD:' THEN DO
            cmd.testcount = STRIP(SUBSTR(line, 5))
        END
        WHEN LEFT(line, 16) = 'EXPECT_CONTAINS:' THEN DO
            expect.testcount = STRIP(SUBSTR(line, 17))
            expect_mode.testcount = 'CONTAINS'
        END
        WHEN LEFT(line, 10) = 'EXPECT_RC:' THEN DO
            expect_rc.testcount = STRIP(SUBSTR(line, 11))
        END
        WHEN LEFT(line, 7) = 'EXPECT:' THEN DO
            expect.testcount = STRIP(SUBSTR(line, 8))
        END
        OTHERWISE NOP
    END
END
CALL CLOSE('tf')

SAY 'Parsed' testcount 'test cases'

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
    texpect = expect.i
    outfile = 'T:test_out_' || i || '.txt'

    SAY 'Test' i || ':' tdesc

    /* Check if this is an interactive test (ITEST: block) */
    tinteractive = interactive.i
    IF tinteractive = 1 THEN DO
        /* ============================================================
         * Interactive test execution path (ADR-023)
         *
         * 1. Write wrapper script that runs LAUNCH command + captures RC
         * 2. Launch in background via Run
         * 3. Wait for program to initialize (3 seconds)
         * 4. Inject keystrokes via KeyInject
         * 5. Wait for program to exit (3 seconds)
         * 6. Force-kill if still running (Break C)
         * 7. Read RC from wrapper script output
         * ============================================================ */
        scriptfile = 'T:itest_cmd_' || i
        rcfile = 'T:itest_rc_' || i
        clinumfile = 'T:itest_cli_' || i

        /* Defensive: check LAUNCH and KEYS are defined */
        tlaunch = launch.i
        tkeys = keys.i
        IF tlaunch = 'LAUNCH.' || i THEN DO
            CALL WRITELN('rf', 'not ok' i '- ' || tdesc || ' (no LAUNCH defined)')
            failed = failed + 1
            ITERATE
        END
        IF tkeys = 'KEYS.' || i THEN DO
            CALL WRITELN('rf', 'not ok' i '- ' || tdesc || ' (no KEYS defined)')
            failed = failed + 1
            ITERATE
        END

        /* Check KeyInject binary exists */
        IF ~EXISTS('WORK:KeyInject') THEN DO
            CALL WRITELN('rf', 'ok' i '- ' || tdesc || ' # SKIP KeyInject not available')
            passed = passed + 1
            ITERATE
        END

        /* Write Execute script: run command, capture RC */
        IF OPEN('scr', scriptfile, 'W') THEN DO
            CALL WRITELN('scr', 'FailAt 21')
            CALL WRITELN('scr', 'Stack 262144')
            CALL WRITELN('scr', tlaunch)
            CALL WRITELN('scr', 'Echo >' || rcfile || ' $RC')
            CALL CLOSE('scr')
        END

        /* Launch in a NEW console window so the test harness output
         * does not pollute the interactive program's display.
         * CON: opens a new Shell window; the Execute runs inside it.
         * The window auto-closes when the script finishes (AUTO). */
        ADDRESS COMMAND 'Run >' || clinumfile || ' NewShell CON:0/0/640/256/ITEST/AUTO FROM' scriptfile

        /* Wait for program to initialize (3 seconds) */
        ADDRESS COMMAND 'Wait 3'

        /* Inject keystrokes via KeyInject.
         * Do NOT use SAY here -- it writes to the shared console and
         * contaminates the interactive program's display. */
        ADDRESS COMMAND 'WORK:KeyInject' tkeys

        /* Signal host to take a screenshot (visual verification).
         * Write sentinel; host captures screenshot and deletes it.
         * Wait up to 5s for host to process — if sentinel still
         * exists after 5s, host screenshot capture is not active. */
        /* Write screenshot sentinel to WORK: volume.
         * Use ARexx OPEN/CLOSE (not Echo redirect — ADDRESS COMMAND
         * does not pass through a shell for redirect parsing). */
        screenshotfile = 'RESULTS:ss' || i
        IF OPEN('ssf', screenshotfile, 'W') THEN DO
            CALL WRITELN('ssf', 'ready')
            CALL CLOSE('ssf')
        END
        DO ss_wait = 1 TO 10
            IF ~EXISTS(screenshotfile) THEN LEAVE
            ADDRESS COMMAND 'Wait 1'
        END

        /* Wait for program to process quit key and exit (3 seconds) */
        ADDRESS COMMAND 'Wait 3'

        /* If program is still running, force-kill via Break C */
        IF OPEN('cf', clinumfile, 'R') THEN DO
            cliline = READLN('cf')
            CALL CLOSE('cf')
            PARSE VAR cliline '[CLI' clinum ']'
            clinum = STRIP(clinum)
            IF DATATYPE(clinum, 'W') THEN DO
                SAY '  Force-killing CLI' clinum
                ADDRESS COMMAND 'Break' clinum 'C'
            END
        END
        ADDRESS COMMAND 'Wait 1'

        /* Read RC from wrapper script */
        cmdrc = 0
        IF OPEN('rcf', rcfile, 'R') THEN DO
            rcline = READLN('rcf')
            CALL CLOSE('rcf')
            IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
        END

        /* Cleanup temp files */
        ADDRESS COMMAND 'Delete >NIL:' scriptfile
        ADDRESS COMMAND 'Delete >NIL:' rcfile
        ADDRESS COMMAND 'Delete >NIL:' clinumfile

        /* Let background CLI fully exit before next test.
         * Without this, residual processes consume memory and
         * the next interactive test may fail with OOM. */
        ADDRESS COMMAND 'Wait 2'

        /* For interactive tests, output match is always automatic
         * (no stdout capture). Only RC is checked. */
        actual = ''
        match = 1
    END
    ELSE DO
    /* ============================================================
     * Non-interactive test execution path (existing logic)
     * ============================================================ */

    /* Defensive check: skip test if CMD was never defined.
     * Note: SYMBOL('cmd.i') checks the literal name CMD.I, not the
     * compound variable cmd.<value-of-i>. We must construct the
     * symbol name dynamically. */
    tcmd = cmd.i
    IF tcmd = 'CMD.' || i THEN DO
        CALL WRITELN('rf', 'not ok' i '- ' || tdesc || ' (no CMD defined)')
        failed = failed + 1
        ITERATE
    END

    /* Run the command, redirect output to temp file.
     * Use a shell script via Execute with FailAt 21 to ensure
     * stdout is captured even when commands return non-zero RC
     * (e.g., diff returns RC=5 when files differ).
     * The command's actual RC is written to a temp file because
     * Execute's RC only reflects whether Execute itself succeeded
     * (FailAt 21 suppresses the command's RC from propagating).
     *
     * For infinite-output programs (yes, tail -f), use TIMEOUT: N
     * in test cases. The CMD should call WORK:run-with-timeout.rexx
     * which handles backgrounding, breaking, and output capture.
     * See toolchain/templates/run-with-timeout.rexx. */
    scriptfile = 'T:test_cmd_' || i
    rcfile = 'T:test_rc_' || i
    IF OPEN('scr', scriptfile, 'W') THEN DO
        CALL WRITELN('scr', 'FailAt 21')
        CALL WRITELN('scr', 'Stack 262144')
        CALL WRITELN('scr', tcmd '>' outfile)
        CALL WRITELN('scr', 'Echo >' || rcfile || ' $RC')
        CALL CLOSE('scr')
    END
    ADDRESS COMMAND 'Execute' scriptfile

    /* Read the actual command return code from the RC file */
    cmdrc = 0
    IF OPEN('rcf', rcfile, 'R') THEN DO
        rcline = READLN('rcf')
        CALL CLOSE('rcf')
        IF DATATYPE(STRIP(rcline), 'W') THEN cmdrc = STRIP(rcline)
    END
    ADDRESS COMMAND 'Delete >NIL:' scriptfile
    ADDRESS COMMAND 'Delete >NIL:' rcfile

    /* Read actual output -- read ALL lines so EXPECT_CONTAINS works on
     * multi-line output (e.g., unified diff with @@ markers on later lines) */
    actual = ''
    IF OPEN('of', outfile, 'R') THEN DO
        DO WHILE ~EOF('of')
            line = READLN('of')
            IF actual = '' THEN
                actual = line
            ELSE
                actual = actual || '0a'x || line
        END
        CALL CLOSE('of')
    END

    END /* ELSE (non-interactive) */

    /* Compare -- supports exact match (EXPECT:), substring (EXPECT_CONTAINS:),
     * and exit code assertion (EXPECT_RC:). */
    tmode = expect_mode.i
    IF tmode = 'EXPECT_MODE.' || i THEN tmode = 'EXACT'

    /* Check output match -- skip if no EXPECT: or EXPECT_CONTAINS: was set.
     * When only EXPECT_RC: is specified, any output is acceptable. */
    match = 0
    IF texpect = 'EXPECT.' || i THEN DO
        /* No EXPECT line was set -- output match is automatic */
        match = 1
    END
    ELSE IF tmode = 'CONTAINS' THEN DO
        /* Substring search across full multi-line output */
        IF POS(STRIP(texpect), actual) > 0 THEN match = 1
    END
    ELSE DO
        /* Exact match: compare against first line only (backward compat) */
        PARSE VAR actual firstline '0a'x .
        IF STRIP(firstline) = STRIP(texpect) THEN match = 1
    END

    /* Check exit code if EXPECT_RC: was specified */
    rc_ok = 1
    trc = expect_rc.i
    IF trc ~= 'EXPECT_RC.' || i THEN DO
        /* EXPECT_RC was set -- verify the command's return code */
        IF cmdrc ~= trc THEN rc_ok = 0
    END

    IF match = 1 & rc_ok = 1 THEN DO
        CALL WRITELN('rf', 'ok' i '- ' || tdesc)
        passed = passed + 1
    END
    ELSE DO
        CALL WRITELN('rf', 'not ok' i '- ' || tdesc)
        IF match = 0 THEN DO
            IF tmode = 'CONTAINS' THEN
                CALL WRITELN('rf', '#   expected to contain:' texpect)
            ELSE
                CALL WRITELN('rf', '#   expected:' texpect)
            CALL WRITELN('rf', '#   actual:  ' actual)
        END
        IF rc_ok = 0 THEN
            CALL WRITELN('rf', '#   expected RC:' trc '  actual RC:' cmdrc)
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
