/*
 * amissl_glue.c -- manual-OpenLibrary AmiSSL integration for amigit.
 *
 * PDR-012 Phase 3. See docs/pdr/012-amigit-https-networking.md and
 * .claude/rules/known-pitfalls.md entry "libamisslauto.a Is a Hard
 * Runtime Dependency".
 *
 * Why manual-open instead of libamisslauto.a: libamisslauto uses a
 * GCC __attribute__((constructor)) that runs BEFORE main() and calls
 * exit(RETURN_FAIL) if AmiSSL isn't installed -- the amigit binary
 * couldn't even print its usage() without AmiSSL present. Manual
 * OpenLibrary lets us graceful-degrade with a friendly
 * "HTTPS not available; run `amiport install amissl`" error.
 *
 * Lifetime: AmiSSLMasterBase + AmiSSLBase are opened on first TLS
 * connect and cached for the process lifetime. amissl_glue_free_cached()
 * tears them down at program exit (registered by the caller via
 * atexit() in amigit.c). SSL_CTX and SSL handles are per-connection
 * and cleaned up in the io->close callback.
 *
 * Teardown order is load-bearing -- must match wget's proven pattern
 * (but wget uses libamisslauto so we derived it from the AmiSSL 5.x
 * SDK autodocs):
 *
 *   per-connection close:
 *     SSL_shutdown (best-effort)
 *     SSL_free      (frees per-connection state)
 *     SSL_CTX_free  (frees context)
 *     closesocket   (underlying TCP)
 *
 *   process exit (atexit):
 *     CloseAmiSSL()      (releases backend library)
 *     CloseLibrary(AmiSSLMasterBase)
 */

/* This file is Amiga-only. The host/vamos build of http_client.c
 * never calls amissl_glue_open_io() -- parser unit tests use
 * http_connect_io() which bypasses the network backend entirely. */
#ifdef __AMIGA__

#include "http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* AmiSSL master: the thin dispatcher that opens the real backend. */
#include <libraries/amisslmaster.h>
#include <proto/amisslmaster.h>

/* OpenSSL API surface -- routed through AmiSSL inline stubs. These
 * headers define SSL_CTX, SSL, and the SSL_* functions as inline
 * wrappers that dispatch via AmiSSLBase. */
#include <proto/amissl.h>

/* AmiSSL-specific helpers (AmiSSLBase, etc.). */
#include <libraries/amissl.h>

/* AmiSSL tag definitions for OpenAmiSSLTags (the modern init API). */
#include <amissl/tags.h>

/* bsdsocket-shim for amiport_closesocket + errno + SocketBase. */
#define AMIPORT_NET_MACROS
#include <amiport-net/socket.h>

/* SocketBase is defined as a global in lib/bsdsocket-shim/src/socket.c.
 * We reference it externally so we can pass it to OpenAmiSSLTags via
 * the AmiSSL_SocketBase tag -- AmiSSL's backend needs to route its
 * internal networking through the same bsdsocket.library handle the
 * rest of amigit uses (otherwise errno gets wedged and SSL_connect
 * sees nonsense). */
extern struct Library *SocketBase;

/* errno is libnix's thread-local (single-threaded on 68k) errno. We
 * pass &errno via AmiSSL_ErrNoPtr so AmiSSL can write through to the
 * same slot our amiport_* socket wrappers read from. */
#include <errno.h>

/* AmiSSLExtBase is the extension library base, filled in by OpenAmiSSLTags
 * via AmiSSL_GetAmiSSLExtBase. We don't call anything from it but the
 * tag must be present for the backend to complete initialization
 * (matches the autoinit_amissl_main.c pattern in upstream AmiSSL).
 * NOTE: <proto/amissl.h> already declares AmiSSLExtBase as an extern
 * struct Library*, so we provide the definition (no static). */
struct Library *AmiSSLExtBase = NULL;

/* ============================================================
 * Cached globals (opened once per process)
 * ============================================================ */

struct Library *AmiSSLMasterBase = NULL;
struct Library *AmiSSLBase = NULL;

/* Cached OpenSSL symbols -- we don't actually cache individual
 * function pointers, the inline headers do that for us via
 * AmiSSLBase. But we do cache an SSL_CTX per process so every
 * connection doesn't reload the trust store. */
/* NOTE: deferred -- Phase 3 uses a fresh SSL_CTX per connection so
 * the graceful-degrade teardown is simpler to reason about.
 * Phase 10 stabilization may hoist this into a cached global. */

/* ============================================================
 * Error reporting -- routed to stderr, visible on Amiga console
 * ============================================================ */

static void
glue_errf(const char *fmt, ...)
{
    va_list ap;
    fputs("amissl_glue: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* ============================================================
 * Library open / close
 * ============================================================ */

/*
 * Ensure AmiSSL is opened. Returns HTTP_OK on success or
 * HTTP_ERR_TLS_MISSING if AmiSSL isn't installed.
 * Safe to call repeatedly -- no-ops after first success.
 *
 * Uses the modern OpenAmiSSLTags flow (matches upstream
 * src/autoinit_amissl_main.c in jens-maus/amissl). The legacy
 * InitAmiSSLMaster + OpenAmiSSL pair that our Phase 3 session 1
 * glue started from DOES NOT WORK on 68k -- the backend needs
 * SocketBase and ErrNoPtr tags to bind its internal networking
 * to the process's bsdsocket handle and errno slot, otherwise
 * OpenAmiSSL returns NULL with no diagnostic.
 *
 * Precondition: SocketBase must be set (amiport_socket_init
 * must have been called). The normal HTTPS call path satisfies
 * this because http_connect calls amiport_getaddrinfo before
 * reaching us. The _https-probe --lib-only diagnostic path
 * calls amiport_socket_init() explicitly.
 *
 * Return-code convention for OpenAmiSSLTags (from upstream
 * autoinit_amissl_main.c):
 *   0 -- success
 *   1 -- couldn't initialise amisslmaster.library
 *   2 -- couldn't open AmiSSL backend
 *   3 -- couldn't initialise AmiSSL
 */
static int
ensure_amissl_open(void)
{
    LONG rc;

    if (AmiSSLMasterBase != NULL && AmiSSLBase != NULL) {
        return HTTP_OK;
    }

    if (SocketBase == NULL) {
        /* Our precondition isn't met -- the caller forgot to
         * initialize bsdsocket before reaching us. Fail cleanly
         * rather than letting OpenAmiSSLTags silently wedge. */
        glue_errf("SocketBase is NULL -- amiport_socket_init not called");
        return HTTP_ERR_TLS_MISSING;
    }

    if (AmiSSLMasterBase == NULL) {
        /* Try LIBS: chain first (standard path). Version 5 is
         * AMISSLMASTER_MIN_VERSION -- matches upstream convention
         * for 68k builds. Fall back to explicit paths for the
         * FS-UAE harness which stages libs into WORK:Libs / RAM:Libs
         * rather than the real LIBS: assign. */
        AmiSSLMasterBase = OpenLibrary(
            (CONST_STRPTR)"amisslmaster.library", AMISSLMASTER_MIN_VERSION);
        if (AmiSSLMasterBase == NULL) {
            AmiSSLMasterBase = OpenLibrary(
                (CONST_STRPTR)"WORK:Libs/amisslmaster.library",
                AMISSLMASTER_MIN_VERSION);
        }
        if (AmiSSLMasterBase == NULL) {
            AmiSSLMasterBase = OpenLibrary(
                (CONST_STRPTR)"RAM:Libs/amisslmaster.library",
                AMISSLMASTER_MIN_VERSION);
        }
        if (AmiSSLMasterBase == NULL) {
            /* Not installed -- caller should print a friendly hint. */
            return HTTP_ERR_TLS_MISSING;
        }
    }

    /* Modern flow: OpenAmiSSLTags does version negotiation,
     * backend load, and init in one call. We pass the process's
     * SocketBase (from libamiport-net) and &errno so the backend
     * shares networking/errno state with our existing bsdsocket
     * handle. AmiSSL_GetAmiSSLBase / AmiSSL_GetAmiSSLExtBase
     * fill in our base-pointer globals. */
    rc = OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                        AmiSSL_UsesOpenSSLStructs, FALSE,
                        AmiSSL_GetAmiSSLBase,    (ULONG)&AmiSSLBase,
                        AmiSSL_GetAmiSSLExtBase, (ULONG)&AmiSSLExtBase,
                        AmiSSL_SocketBase,       (ULONG)SocketBase,
                        AmiSSL_ErrNoPtr,         (ULONG)&errno,
                        TAG_DONE);
    if (rc != 0) {
        const char *reason = "unknown";
        switch (rc) {
        case 1: reason = "master init failed"; break;
        case 2: reason = "backend open failed"; break;
        case 3: reason = "backend init failed"; break;
        }
        glue_errf("OpenAmiSSLTags failed rc=%ld (%s)", (long)rc, reason);
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
        AmiSSLBase = NULL;
        AmiSSLExtBase = NULL;
        return HTTP_ERR_TLS_MISSING;
    }

    return HTTP_OK;
}

/*
 * amissl_glue_probe -- PDR-012 Phase 3 session 3 diagnostic.
 * Runs the same sequence as ensure_amissl_open() but with heavy
 * per-call instrumentation. Prints AvailMem before every attempt
 * and IoErr after every failure. Exercises both name-only LIBS:
 * walk and explicit path fallbacks. Used by `amigit _https-probe
 * --lib-only` to isolate why OpenLibrary returns NULL inside
 * amigit's runtime environment while the standalone amissl-probe
 * binary (zero amiport deps) succeeds at the same calls.
 *
 * Returns 0 on any successful open (with CloseLibrary of the
 * probed handle); returns -1 if ALL open variants fail.
 * Deliberately does NOT cache or chain through InitAmiSSLMaster
 * -- we only want to know if OpenLibrary on the master works.
 */
static void
probe_mem(const char *label)
{
    printf("  mem %s: fast=%lu public=%lu largest=%lu\n",
           label,
           (unsigned long)AvailMem(MEMF_FAST),
           (unsigned long)AvailMem(MEMF_PUBLIC),
           (unsigned long)AvailMem(MEMF_LARGEST));
    fflush(stdout);
}

static struct Library *
probe_open(const char *path, unsigned long version)
{
    struct Library *base;
    printf("  OpenLibrary(%s, v%lu): ", path, version);
    fflush(stdout);
    base = OpenLibrary((CONST_STRPTR)path, version);
    if (base == NULL) {
        printf("NULL ioerr=%ld\n", (long)IoErr());
    } else {
        printf("ok v%ld.%ld\n",
               (long)base->lib_Version,
               (long)base->lib_Revision);
    }
    fflush(stdout);
    return base;
}

int
amissl_glue_probe(void)
{
    LONG rc;

    printf("amissl_glue_probe: OpenAmiSSLTags verification\n");
    probe_mem("start");

    /* ensure_amissl_open needs SocketBase set. Normal HTTPS
     * path gets it from http_connect calling amiport_getaddrinfo
     * first; the --lib-only path skips networking, so we must
     * trigger bsdsocket open explicitly. */
    printf("  amiport_socket_init(): ");
    fflush(stdout);
    if (amiport_socket_init() != 0) {
        printf("FAIL -- bsdsocket.library not available\n");
        return -1;
    }
    printf("ok SocketBase=0x%08lx\n", (unsigned long)SocketBase);
    probe_mem("after-socket-init");

    /* Open the master library explicitly so we can report the result. */
    printf("  OpenLibrary(amisslmaster.library, %d): ",
           AMISSLMASTER_MIN_VERSION);
    fflush(stdout);
    AmiSSLMasterBase = OpenLibrary((CONST_STRPTR)"amisslmaster.library",
                                   AMISSLMASTER_MIN_VERSION);
    if (AmiSSLMasterBase == NULL) {
        AmiSSLMasterBase = OpenLibrary(
            (CONST_STRPTR)"WORK:Libs/amisslmaster.library",
            AMISSLMASTER_MIN_VERSION);
    }
    if (AmiSSLMasterBase == NULL) {
        AmiSSLMasterBase = OpenLibrary(
            (CONST_STRPTR)"RAM:Libs/amisslmaster.library",
            AMISSLMASTER_MIN_VERSION);
    }
    if (AmiSSLMasterBase == NULL) {
        printf("NULL -- master library not found at any path\n");
        return -1;
    }
    printf("ok v%ld.%ld\n",
           (long)AmiSSLMasterBase->lib_Version,
           (long)AmiSSLMasterBase->lib_Revision);
    probe_mem("after-master-open");

    /* Now do the actual OpenAmiSSLTags call with the production tag
     * list, visibly reporting rc. 0=ok, 1=master init, 2=backend open,
     * 3=backend init. */
    printf("  OpenAmiSSLTags(APIVer=%d, UsesStructs=FALSE, ...): ",
           AMISSL_CURRENT_VERSION);
    fflush(stdout);
    rc = OpenAmiSSLTags(AMISSL_CURRENT_VERSION,
                        AmiSSL_UsesOpenSSLStructs, FALSE,
                        AmiSSL_GetAmiSSLBase,    (ULONG)&AmiSSLBase,
                        AmiSSL_GetAmiSSLExtBase, (ULONG)&AmiSSLExtBase,
                        AmiSSL_SocketBase,       (ULONG)SocketBase,
                        AmiSSL_ErrNoPtr,         (ULONG)&errno,
                        TAG_DONE);
    if (rc == 0) {
        printf("ok AmiSSLBase=0x%08lx AmiSSLExtBase=0x%08lx\n",
               (unsigned long)AmiSSLBase,
               (unsigned long)AmiSSLExtBase);
        probe_mem("after-tags");
        CloseAmiSSL();
        AmiSSLBase = NULL;
        AmiSSLExtBase = NULL;
    } else {
        const char *reason = "unknown";
        switch (rc) {
        case 1: reason = "master init failed"; break;
        case 2: reason = "backend open failed"; break;
        case 3: reason = "backend init failed"; break;
        }
        printf("FAIL rc=%ld (%s)\n", (long)rc, reason);
        probe_mem("after-tags-fail");
    }

    CloseLibrary(AmiSSLMasterBase);
    AmiSSLMasterBase = NULL;
    probe_mem("end");
    printf("amissl_glue_probe: %s\n", rc == 0 ? "OK" : "FAIL");
    fflush(stdout);
    return rc == 0 ? 0 : -1;
}

/*
 * amissl_glue_free_cached -- atexit hook. Tear down the cached
 * master/backend handles. Safe to call even if nothing was ever
 * opened (no-op if AmiSSLMasterBase == NULL).
 */
void
amissl_glue_free_cached(void)
{
    /* Modern teardown: CloseAmiSSL releases both AmiSSLBase and
     * AmiSSLExtBase if they were set up via OpenAmiSSLTags. Then
     * we close the master library. Order matches upstream
     * autoinit_amissl_main.c. */
    if (AmiSSLBase != NULL) {
        CloseAmiSSL();
        AmiSSLBase = NULL;
        AmiSSLExtBase = NULL;
    }
    if (AmiSSLMasterBase != NULL) {
        CloseLibrary(AmiSSLMasterBase);
        AmiSSLMasterBase = NULL;
    }
}

/* ============================================================
 * Verify callback -- explicit default behavior
 * ============================================================ */

/*
 * Delegates to OpenSSL's built-in chain verification via preverify_ok.
 * Passing this explicitly (instead of NULL) is functionally identical
 * but documents that we are not overriding the default verify logic.
 * Hostname binding is handled separately by X509_VERIFY_PARAM_set1_host
 * below.
 */
static int
amigit_verify_cb(int preverify_ok, X509_STORE_CTX *ctx)
{
    (void)ctx;
    return preverify_ok;
}

/* ============================================================
 * Per-connection SSL state
 * ============================================================ */

typedef struct {
    SSL_CTX *ctx;
    SSL     *ssl;
    int      sockfd;
} ssl_priv_t;

static int
ssl_read(http_io_t *io, void *buf, int len)
{
    ssl_priv_t *p = (ssl_priv_t *)io->priv;
    int n = SSL_read(p->ssl, buf, len);
    if (n < 0) {
        return HTTP_ERR_RECV;
    }
    return n;
}

static int
ssl_write(http_io_t *io, const void *buf, int len)
{
    ssl_priv_t *p = (ssl_priv_t *)io->priv;
    int n = SSL_write(p->ssl, buf, len);
    if (n < 0) {
        return HTTP_ERR_SEND;
    }
    return n;
}

static void
ssl_close(http_io_t *io)
{
    ssl_priv_t *p = (ssl_priv_t *)io->priv;
    if (p == NULL) {
        return;
    }
    if (p->ssl != NULL) {
        /* Best-effort shutdown; ignore return value. */
        SSL_shutdown(p->ssl);
        SSL_free(p->ssl);
        p->ssl = NULL;
    }
    if (p->ctx != NULL) {
        SSL_CTX_free(p->ctx);
        p->ctx = NULL;
    }
    if (p->sockfd >= 0) {
        amiport_closesocket(p->sockfd);
        p->sockfd = -1;
    }
    free(p);
    io->priv = NULL;
}

/* ============================================================
 * amissl_glue_open_io -- public entry point
 * ============================================================ */

/*
 * On success, the io struct's read/write/close/priv are wired and
 * the caller's sockfd is owned by the io (ssl_close will closesocket
 * it). On failure, the sockfd IS closed here and io is left
 * untouched (caller must free the io struct itself).
 */
int
amissl_glue_open_io(http_io_t *io, int sockfd, const char *host)
{
    ssl_priv_t *p = NULL;
    int rc;

    if (io == NULL || sockfd < 0) {
        if (sockfd >= 0) {
            amiport_closesocket(sockfd);
        }
        return HTTP_ERR_INVAL;
    }

    rc = ensure_amissl_open();
    if (rc < 0) {
        amiport_closesocket(sockfd);
        return rc;  /* HTTP_ERR_TLS_MISSING */
    }

    p = (ssl_priv_t *)calloc(1, sizeof(*p));
    if (p == NULL) {
        amiport_closesocket(sockfd);
        return HTTP_ERR_NOMEM;
    }
    p->sockfd = sockfd;
    p->ctx = NULL;
    p->ssl = NULL;

    /* Create a fresh CTX per connection (see note in cached globals
     * section -- Phase 10 may hoist this). TLS_client_method() is
     * the modern catch-all that picks the best available version. */
    p->ctx = SSL_CTX_new(TLS_client_method());
    if (p->ctx == NULL) {
        glue_errf("SSL_CTX_new failed");
        amiport_closesocket(p->sockfd);
        free(p);
        return HTTP_ERR_TLS_HANDSHAKE;
    }

    /* Require the full TLS peer verify chain AND hostname binding.
     * SSL_VERIFY_PEER alone would only verify the chain -- a cert
     * issued for *any* trusted domain would pass. We also bind the
     * expected hostname via X509_VERIFY_PARAM_set1_host so a
     * github.com cert on amiport.platesteel.net is rejected at
     * handshake time. AmiSSL 5.x exposes the OpenSSL 3.x API so
     * X509_VERIFY_PARAM_* is available. */
    SSL_CTX_set_verify(p->ctx, SSL_VERIFY_PEER, amigit_verify_cb);
    SSL_CTX_set_default_verify_paths(p->ctx);

    p->ssl = SSL_new(p->ctx);
    if (p->ssl == NULL) {
        glue_errf("SSL_new failed");
        SSL_CTX_free(p->ctx);
        amiport_closesocket(p->sockfd);
        free(p);
        return HTTP_ERR_TLS_HANDSHAKE;
    }

    /* Hostname verification: bind the expected host to the SSL
     * object's verify params BEFORE SSL_connect so the verify
     * callback rejects cert/host mismatches. A failure here is a
     * hard error, not a warning. Also set the SNI hostname so
     * virtual-hosted HTTPS servers (github.com,
     * amiport.platesteel.net) return the right cert in the first
     * place. */
    if (host != NULL && *host != '\0') {
        X509_VERIFY_PARAM *vp = SSL_get0_param(p->ssl);
        if (vp != NULL) {
            X509_VERIFY_PARAM_set_hostflags(vp,
                X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            if (X509_VERIFY_PARAM_set1_host(vp, host, 0) != 1) {
                glue_errf("X509_VERIFY_PARAM_set1_host failed (host=%s)",
                          host);
                SSL_free(p->ssl);
                SSL_CTX_free(p->ctx);
                amiport_closesocket(p->sockfd);
                free(p);
                return HTTP_ERR_TLS_HANDSHAKE;
            }
        }
        (void)SSL_set_tlsext_host_name(p->ssl, host);
    }

    if (SSL_set_fd(p->ssl, p->sockfd) != 1) {
        glue_errf("SSL_set_fd failed");
        SSL_free(p->ssl);
        SSL_CTX_free(p->ctx);
        amiport_closesocket(p->sockfd);
        free(p);
        return HTTP_ERR_TLS_HANDSHAKE;
    }

    if (SSL_connect(p->ssl) != 1) {
        glue_errf("SSL_connect failed (host=%s)", host ? host : "?");
        SSL_free(p->ssl);
        SSL_CTX_free(p->ctx);
        amiport_closesocket(p->sockfd);
        free(p);
        return HTTP_ERR_TLS_HANDSHAKE;
    }

    /* Success -- wire the io vtable. From here on the ssl_close
     * callback owns all the resources. */
    io->read  = ssl_read;
    io->write = ssl_write;
    io->close = ssl_close;
    io->priv  = p;

    return HTTP_OK;
}

#else /* !__AMIGA__ -- host/vamos build */

#include "http_client.h"

/* Host build: the glue isn't linkable (no AmiSSL headers) but we
 * still need the symbol so http_client.c links. Parser unit tests
 * never call these. */

int
amissl_glue_open_io(struct http_io *io, int sockfd, const char *host)
{
    (void)io;
    (void)sockfd;
    (void)host;
    return -3; /* HTTP_ERR_TLS_MISSING */
}

void
amissl_glue_free_cached(void)
{
}

int
amissl_glue_probe(void)
{
    return -1;
}

#endif /* __AMIGA__ */
