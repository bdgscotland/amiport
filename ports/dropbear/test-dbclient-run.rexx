/* test-dbclient-run.rexx -- Run dbclient with env-based password
 *
 * Usage: rx WORK:test-dbclient-run.rexx <password> <host> <command>
 *
 * Sets DROPBEAR_PASSWORD env var, runs dbclient -y with the given
 * command, outputs the result. This avoids the getpass() issue. */

PARSE ARG password host rcmd

IF password = '' THEN DO
    SAY 'Usage: rx test-dbclient-run.rexx <password> <host> <command>'
    EXIT 10
END

IF host = '' THEN host = '127.0.0.1'

/* Set the password env var */
ADDRESS COMMAND 'SetEnv DROPBEAR_PASSWORD' password

/* Run dbclient with auto-accept host key, non-interactive */
IF rcmd ~= '' THEN
    ADDRESS COMMAND 'WORK:dbclient -y duncan@' || host rcmd
ELSE
    ADDRESS COMMAND 'WORK:dbclient -y duncan@' || host

cmdrc = RC

/* Clean up password from environment */
ADDRESS COMMAND 'UnSetEnv DROPBEAR_PASSWORD'

EXIT cmdrc
