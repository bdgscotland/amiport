/* test-touch-realworld.rexx -- real-world: create sentinel/lock files pattern */
/* Usage: rx WORK:test-touch-realworld.rexx */
/* Simulates a build system creating marker files to track completed steps */
/* Prints "OK" on success, "FAIL: reason" on failure */
OPTIONS FAILAT 21

step1 = 'T:touch_rw_step1.done'
step2 = 'T:touch_rw_step2.done'
step3 = 'T:touch_rw_step3.done'
lock  = 'T:touch_rw_build.lock'

/* Clean up leftovers */
ADDRESS COMMAND 'Delete >NIL: ' || step1
ADDRESS COMMAND 'Delete >NIL: ' || step2
ADDRESS COMMAND 'Delete >NIL: ' || step3
ADDRESS COMMAND 'Delete >NIL: ' || lock

/* Step 1: create build lock file */
ADDRESS COMMAND 'WORK:touch ' || lock
IF ~EXISTS(lock) THEN DO
    SAY 'FAIL: could not create lock file'
    EXIT 10
END

/* Step 2: create step completion markers */
ADDRESS COMMAND 'WORK:touch ' || step1 || ' ' || step2 || ' ' || step3
trc = RC

IF trc ~= 0 THEN DO
    SAY 'FAIL: touch step markers returned RC=' || trc
    ADDRESS COMMAND 'Delete >NIL: ' || lock
    EXIT 10
END

/* Verify all steps marked complete */
ok = 1
IF ~EXISTS(step1) THEN ok = 0
IF ~EXISTS(step2) THEN ok = 0
IF ~EXISTS(step3) THEN ok = 0

/* Step 3: update lock file timestamp to signal completion */
ADDRESS COMMAND 'WORK:touch ' || lock
trc = RC

/* Clean up */
ADDRESS COMMAND 'Delete >NIL: ' || step1
ADDRESS COMMAND 'Delete >NIL: ' || step2
ADDRESS COMMAND 'Delete >NIL: ' || step3
ADDRESS COMMAND 'Delete >NIL: ' || lock

IF ok = 0 THEN DO
    SAY 'FAIL: not all step markers created'
    EXIT 10
END

IF trc ~= 0 THEN DO
    SAY 'FAIL: updating lock file returned RC=' || trc
    EXIT 10
END

SAY 'OK'
EXIT 0
