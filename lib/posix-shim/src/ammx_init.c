/* ammx_init.c -- Apollo 68080 AMMX2 initialization.
 * Wraps the canonical pattern from arczi84/NetSurf-MUI's jsimd_ammx.c
 * and the Apollo team's V_EnableAMMX documentation.
 */

#include <amiport/ammx.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <proto/exec.h>

#include <vampire/vampire.h>
#include <proto/vampire.h>

extern struct ExecBase *SysBase;

/* Bit position in ExecBase->AttnFlags for Apollo 68080 detection.
 * Per Apollo PRM and known-pitfalls AMMX section. */
#ifndef AFB_68080
#define AFB_68080 10
#endif

#ifndef AFF_68080
#define AFF_68080 (1 << AFB_68080)
#endif

static int g_ammx_init_status = -1;

int amiport_ammx_init(void)
{
    struct Library *VampireBase;

    if (g_ammx_init_status != -1) {
        return g_ammx_init_status;
    }

    /* Step 1: confirm Apollo silicon present */
    if (!(SysBase->AttnFlags & AFF_68080)) {
        g_ammx_init_status = 1;
        return g_ammx_init_status;
    }

    /* Step 2: open vampire.resource */
    VampireBase = (struct Library *)OpenResource((CONST_STRPTR)V_VAMPIRENAME);
    if (!VampireBase) {
        g_ammx_init_status = 2;
        return g_ammx_init_status;
    }

    /* Step 3: confirm version >= 45 (V_EnableAMMX entry point) */
    if (VampireBase->lib_Version < 45) {
        g_ammx_init_status = 3;
        return g_ammx_init_status;
    }

    /* Step 4: enable AMMX2 context-switch handling for this task */
    if (V_EnableAMMX(V_AMMX_V2) == VRES_ERROR) {
        g_ammx_init_status = 4;
        return g_ammx_init_status;
    }

    g_ammx_init_status = 0;
    return g_ammx_init_status;
}

int amiport_ammx_status(void)
{
    return g_ammx_init_status;
}
