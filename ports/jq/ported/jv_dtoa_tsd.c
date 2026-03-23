#include <stdlib.h>
#include <stdio.h>

#include "jv_dtoa.h"
#include "jv_dtoa_tsd.h"

/* amiport: replaced pthread TSD with static singleton — AmigaOS is single-threaded */

static struct dtoa_context dtoa_ctx_static;
static int dtoa_ctx_inited = 0;

/* amiport: jvp_dtoa_context_fini doesn't exist in jv_dtoa.c — dtoa context
   is static data that doesn't need cleanup. Removed atexit fini call. */

struct dtoa_context *tsd_dtoa_context_get(void) {
  if (!dtoa_ctx_inited) {
    jvp_dtoa_context_init(&dtoa_ctx_static);
    dtoa_ctx_inited = 1;
  }
  return &dtoa_ctx_static;
}
