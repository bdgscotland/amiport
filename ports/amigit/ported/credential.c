/*
 * credential.c -- HTTP Basic auth credential sourcing for amigit.
 *
 * See credential.h for architecture notes. This TU does three
 * things and nothing else: env-var read, interactive prompt,
 * base64 encode. All public functions work on caller-owned
 * buffers and zero their internal scratch before returning.
 *
 * amiport: PDR-012 Phase 7 (2026-04-15).
 */

#include "credential.h"

#include <proto/dos.h>
#include <dos/dos.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* ========================================================================
 * amigit_credential_zero -- compiler-proof memory scrub
 *
 * A plain memset() is legal to eliminate under the as-if rule if
 * the buffer is not observed after the call. This volatile loop
 * cannot be optimized away and is the standard idiom for wiping
 * credential material.
 * ======================================================================== */

void
amigit_credential_zero(void *buf, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)buf;
    while (len > 0) {
        *p++ = 0;
        len--;
    }
}

/* ========================================================================
 * amigit_base64_encode -- RFC 4648 section 4, standard alphabet
 *
 * Standard 3-byte-to-4-char encoder with '=' padding. No line
 * wrapping (HTTP Basic headers don't want it), ASCII alphabet
 * only. Validated against known vectors:
 *   "" -> ""
 *   "f" -> "Zg=="
 *   "fo" -> "Zm8="
 *   "foo" -> "Zm9v"
 *   "foob" -> "Zm9vYg=="
 *   "fooba" -> "Zm9vYmE="
 *   "foobar" -> "Zm9vYmFy"
 * The FS-UAE test suite exercises the "git:token" shape directly.
 * ======================================================================== */

int
amigit_base64_encode(const void *src, size_t src_len,
                     char *dst, size_t dst_sz)
{
    static const char alphabet[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    const unsigned char *s;
    size_t out_pos;
    size_t needed;
    size_t i;

    if (dst == NULL || dst_sz == 0) {
        return -1;
    }
    needed = ((src_len + 2) / 3) * 4 + 1;
    if (dst_sz < needed) {
        return -1;
    }
    if (src == NULL && src_len != 0) {
        return -1;
    }

    s = (const unsigned char *)src;
    out_pos = 0;

    /* Full 3-byte groups. */
    i = 0;
    while (i + 3 <= src_len) {
        unsigned long v =
            ((unsigned long)s[i]     << 16) |
            ((unsigned long)s[i + 1] <<  8) |
            ((unsigned long)s[i + 2]);
        dst[out_pos++] = alphabet[(v >> 18) & 0x3F];
        dst[out_pos++] = alphabet[(v >> 12) & 0x3F];
        dst[out_pos++] = alphabet[(v >>  6) & 0x3F];
        dst[out_pos++] = alphabet[ v        & 0x3F];
        i += 3;
    }

    /* Trailing 1 or 2 bytes + padding. */
    if (i < src_len) {
        unsigned long v;
        int two_bytes = (i + 1 < src_len);

        v = (unsigned long)s[i] << 16;
        if (two_bytes) {
            v |= (unsigned long)s[i + 1] << 8;
        }
        dst[out_pos++] = alphabet[(v >> 18) & 0x3F];
        dst[out_pos++] = alphabet[(v >> 12) & 0x3F];
        dst[out_pos++] = two_bytes ? alphabet[(v >> 6) & 0x3F] : '=';
        dst[out_pos++] = '=';
    }

    dst[out_pos] = '\0';
    return (int)out_pos;
}

/* ========================================================================
 * read_env_var -- fetch an AmigaDOS env variable from ENV:<name>
 *
 * Does NOT use GetVar() or libnix getenv() because both are
 * unreliable in some process contexts (see known-pitfalls "libnix
 * getenv() and GetVar() don't reliably read ENV: files at
 * runtime"). Opens ENV:<name> via AmigaDOS Open/Read/Close
 * directly, matching the Dropbear port's env var pattern.
 *
 * On success writes NUL-terminated value to buf and returns the
 * string length. On failure (file missing, buf too small, read
 * error) returns -1 and leaves buf[0] = '\0'.
 *
 * Trims trailing CR/LF so `SetEnv VAR value` (which writes a
 * trailing newline via `echo`) works transparently.
 * ======================================================================== */

static int
read_env_var(const char *name, char *buf, size_t buf_sz)
{
    char path[128];
    BPTR fh;
    LONG n;
    size_t len;
    int rc;

    if (name == NULL || buf == NULL || buf_sz < 2) {
        if (buf != NULL && buf_sz > 0) {
            buf[0] = '\0';
        }
        return -1;
    }
    buf[0] = '\0';

    rc = snprintf(path, sizeof(path), "ENV:%s", name);
    if (rc < 0 || (size_t)rc >= sizeof(path)) {
        return -1;
    }

    fh = Open((CONST_STRPTR)path, MODE_OLDFILE);
    if (fh == (BPTR)0) {
        return -1;
    }

    n = Read(fh, (APTR)buf, (LONG)(buf_sz - 1));
    Close(fh);
    if (n <= 0) {
        buf[0] = '\0';
        return -1;
    }
    if ((size_t)n >= buf_sz) {
        n = (LONG)(buf_sz - 1);
    }
    buf[n] = '\0';

    /* Trim trailing newline / carriage return. */
    len = (size_t)n;
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }

    if (len == 0) {
        return -1;
    }
    return (int)len;
}

/* ========================================================================
 * prompt_interactive -- read credentials from the console
 *
 * Username: cooked-mode fgets with echo.
 * Token: raw-mode Read() loop with no echo. Backspace (0x08 /
 * 0x7F) trims the last char. Enter (CR or LF) terminates.
 *
 * Returns 0 on success, -1 if Input() is not interactive, the
 * raw-mode switch fails and we can't safely read the token, or
 * either buffer ends up empty.
 *
 * NOTE: a known pitfall says "Do NOT Open('*') in library init
 * code". This function is NOT library init code -- it runs inside
 * the credential retry path after the http client has already
 * been live. Opening "*" here would still be risky because we're
 * mid-request, so we use Input() directly instead.
 * ======================================================================== */

static int
prompt_interactive(char *user_buf, size_t user_buf_sz,
                   char *token_buf, size_t token_buf_sz)
{
    BPTR fh;
    size_t pos;
    char c;
    LONG r;
    int raw_active;

    if (user_buf == NULL || user_buf_sz < 4 ||
        token_buf == NULL || token_buf_sz < 2) {
        return -1;
    }
    user_buf[0]  = '\0';
    token_buf[0] = '\0';

    fh = Input();
    if (fh == (BPTR)0 || !IsInteractive(fh)) {
        return -1;
    }

    /* Username prompt. Cooked mode is the default; fgets handles
     * line editing and echo via the standard console handler. */
    printf("GitHub username [git]: ");
    fflush(stdout);
    if (fgets(user_buf, (int)user_buf_sz, stdin) == NULL) {
        amigit_credential_zero(user_buf, user_buf_sz);
        return -1;
    }
    {
        size_t n = strlen(user_buf);
        while (n > 0 &&
               (user_buf[n - 1] == '\n' || user_buf[n - 1] == '\r')) {
            user_buf[--n] = '\0';
        }
        if (n == 0) {
            /* GitHub's PAT convention: username is literally "git"
             * and the PAT fills the password slot. */
            (void)strcpy(user_buf, "git");
        }
    }

    /* Token prompt. RAW mode suppresses echo so the PAT isn't
     * visible on the console. Read one byte at a time until Enter,
     * handle backspace for line editing. */
    printf("Personal access token (will not be echoed): ");
    fflush(stdout);

    raw_active = 0;
    if (SetMode(fh, 1) == DOSTRUE) {
        raw_active = 1;
    }

    pos = 0;
    if (!raw_active) {
        /* Fallback: if RAW mode can't be enabled, read the token
         * echoed. A visible PAT is worse than no PAT, but failing
         * outright is worse still. The user sees their PAT on
         * screen -- they can clear the scrollback after.
         *
         * This path should never fire in practice: console
         * SetMode(1) is reliably supported by AmigaOS 1.x+. */
        if (fgets(token_buf, (int)token_buf_sz, stdin) == NULL) {
            amigit_credential_zero(token_buf, token_buf_sz);
            return -1;
        }
        {
            size_t n = strlen(token_buf);
            while (n > 0 &&
                   (token_buf[n - 1] == '\n' ||
                    token_buf[n - 1] == '\r')) {
                token_buf[--n] = '\0';
            }
            if (n == 0) {
                return -1;
            }
        }
    } else {
        for (;;) {
            r = Read(fh, (APTR)&c, 1);
            if (r <= 0) {
                break;
            }
            if (c == '\r' || c == '\n') {
                break;
            }
            if (c == 0x08 || c == 0x7F) {
                /* Backspace: drop last char silently (no echo). */
                if (pos > 0) {
                    pos--;
                }
                continue;
            }
            if (pos + 1 < token_buf_sz) {
                token_buf[pos++] = c;
            }
            /* else: silently drop trailing chars beyond capacity */
        }
        token_buf[pos] = '\0';

        (void)SetMode(fh, 0);

        if (pos == 0) {
            return -1;
        }
    }

    /* We suppressed the user's Enter in raw mode -- print our own
     * newline so subsequent output lands on a fresh line. Harmless
     * to print in the fgets fallback path too (fgets already
     * consumed its own newline). */
    printf("\n");
    fflush(stdout);

    return 0;
}

/* ========================================================================
 * amigit_credential_get -- public entry point
 *
 * Checks env vars first, falls back to interactive prompt, fails
 * with a user-actionable error if neither source produces a token.
 * ======================================================================== */

int
amigit_credential_get(char *user_buf, size_t user_buf_sz,
                      char *token_buf, size_t token_buf_sz,
                      char *errbuf, size_t errbuf_sz)
{
    int ulen;
    int tlen;

    if (user_buf == NULL || user_buf_sz < 4 ||
        token_buf == NULL || token_buf_sz < 2 ||
        errbuf == NULL || errbuf_sz == 0) {
        if (errbuf != NULL && errbuf_sz > 0) {
            (void)snprintf(errbuf, errbuf_sz,
                "amigit: internal: bad args to amigit_credential_get");
        }
        return -1;
    }
    user_buf[0]  = '\0';
    token_buf[0] = '\0';

    /* Source 1: environment variables. */
    tlen = read_env_var("GIT_HTTP_TOKEN", token_buf, token_buf_sz);
    if (tlen > 0) {
        ulen = read_env_var("GIT_HTTP_USERNAME", user_buf, user_buf_sz);
        if (ulen <= 0) {
            /* Token set, username missing -> GitHub PAT convention. */
            if (user_buf_sz < 4) {
                amigit_credential_zero(token_buf, token_buf_sz);
                (void)snprintf(errbuf, errbuf_sz,
                    "amigit: user buffer too small for default username");
                return -1;
            }
            (void)strcpy(user_buf, "git");
        }
        return 0;
    }

    /* Source 2: interactive prompt. Only if stdin is a tty. */
    if (prompt_interactive(user_buf, user_buf_sz,
                           token_buf, token_buf_sz) == 0) {
        return 0;
    }

    /* Neither source produced credentials -- tell the user how to
     * fix it without leaking any partial state. */
    amigit_credential_zero(user_buf,  user_buf_sz);
    amigit_credential_zero(token_buf, token_buf_sz);
    (void)snprintf(errbuf, errbuf_sz,
        "amigit: HTTPS auth required but no credentials available. "
        "Set ENV:GIT_HTTP_TOKEN (SetEnv GIT_HTTP_TOKEN <pat>) "
        "and optionally ENV:GIT_HTTP_USERNAME, or run interactively "
        "so amigit can prompt.");
    return -1;
}
