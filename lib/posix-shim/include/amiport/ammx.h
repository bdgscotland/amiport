/* amiport/ammx.h -- Apollo 68080 AMMX2 initialization wrapper.
 *
 * MUST be called once at process startup before any AMMX kernel runs.
 * Without this, AMMX2-aware code (E0-E23 register file, AMMX2 opcodes)
 * will not be saved correctly across context switches and the system
 * will Guru on the first task switch under load.
 *
 * This is the canonical entry point for amiport ports that link
 * vasm-assembled AMMX kernels.
 *
 * Hardware-required: there is no scalar fallback. If your port wants to
 * support stock 68k systems, it must check this return value and exit
 * cleanly (or use a non-AMMX code path managed by the port itself).
 */

#ifndef AMIPORT_AMMX_H
#define AMIPORT_AMMX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize AMMX2 context-switch handling for the current task.
 * Returns 0 on success, non-zero on failure:
 *   1 = no Apollo 68080 detected (AFB_68080 not in ExecBase->AttnFlags)
 *   2 = vampire.resource missing (V_VAMPIRENAME OpenResource failed)
 *   3 = vampire.resource version too old (< 45)
 *   4 = V_EnableAMMX(V_AMMX_V2) returned VRES_ERROR
 *
 * Safe to call multiple times -- second and subsequent calls are no-ops
 * that re-return the cached first-call result.
 */
int amiport_ammx_init(void);

/* Returns the cached result of the most recent amiport_ammx_init call.
 * Returns -1 if amiport_ammx_init has never been called.
 */
int amiport_ammx_status(void);

#ifdef __cplusplus
}
#endif

#endif /* AMIPORT_AMMX_H */
