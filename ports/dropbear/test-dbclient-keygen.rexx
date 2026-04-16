/* test-dbclient-keygen.rexx -- Generate SSH key and show pubkey
 *
 * Usage: rx WORK:test-dbclient-keygen.rexx [keytype]
 *
 * Creates S:.ssh/ directory if needed, generates an SSH key pair,
 * and prints the public key for adding to the server's
 * authorized_keys file.
 *
 * Key types: ed25519 (default), rsa, ecdsa
 * Key file:  WORK:.ssh/id_dropbear
 */

OPTIONS FAILAT 21

PARSE ARG keytype

IF keytype = '' THEN keytype = 'ed25519'

keydir = 'WORK:.ssh'
keyfile = keydir || '/id_dropbear'

/* Create S:.ssh/ directory if it does not exist */
ADDRESS COMMAND 'MakeDir >NIL:' keydir

/* Check if key already exists */
IF OPEN('kf', keyfile, 'R') THEN DO
    CALL CLOSE('kf')
    SAY 'Key already exists at' keyfile
    SAY 'To regenerate, delete it first:'
    SAY '  Delete' keyfile
    SAY ''
    SAY 'Showing existing public key:'
    SAY ''
    ADDRESS COMMAND 'WORK:dropbearkey -y -f' keyfile
    EXIT 0
END

/* Generate key */
SAY 'Generating' keytype 'key at' keyfile '...'
SAY ''

ADDRESS COMMAND 'WORK:dropbearkey -t' keytype '-f' keyfile

IF RC > 0 THEN DO
    SAY 'Key generation failed (RC=' || RC || ')'
    EXIT 10
END

/* Save public key to WORK: where the host can read it */
ADDRESS COMMAND 'WORK:dropbearkey -y -f' keyfile '>WORK:dropbear_pubkey.txt'

SAY ''
SAY 'Key generated. Public key also saved to WORK:dropbear_pubkey.txt'
SAY ''
SAY 'To install the public key on the server, run on your Mac:'
SAY '  cat build/amiga/dropbear_pubkey.txt >> ~/.ssh/authorized_keys'
SAY ''
SAY 'Then connect without password:'
SAY '  WORK:dbclient -y -i' keyfile 'user@host'

EXIT 0
