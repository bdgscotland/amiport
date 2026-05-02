/* test_ammx_init.c -- smoke test for amiport_ammx_init().
 * Expected behavior:
 *   - On non-Apollo systems (vamos, FS-UAE without Apollo emulation): returns non-zero
 *   - On real Apollo A6000: returns 0
 * The test prints the result code for inspection; it does not assert
 * a specific value because we want to run it on both Apollo and non-Apollo
 * to verify both paths.
 */

#include <stdio.h>
#include <stdlib.h>
#include <amiport/ammx.h>

long __stack = 262144;

int main(void)
{
    int rc1, rc2;

    printf("amiport_ammx_init test\n");
    printf("======================\n");

    rc1 = amiport_ammx_init();
    printf("First call:  rc=%d ", rc1);
    switch (rc1) {
        case 0: printf("(SUCCESS -- AMMX2 enabled)\n"); break;
        case 1: printf("(no Apollo 68080 -- expected on stock 68k)\n"); break;
        case 2: printf("(vampire.resource missing -- expected on non-Vampire)\n"); break;
        case 3: printf("(vampire.resource too old -- V<45)\n"); break;
        case 4: printf("(V_EnableAMMX failed -- driver issue)\n"); break;
        default: printf("(unexpected code)\n"); break;
    }

    /* Second call should return cached value */
    rc2 = amiport_ammx_init();
    if (rc1 != rc2) {
        printf("FAIL: second call returned different value (%d vs %d)\n", rc1, rc2);
        return 10;
    }

    if (amiport_ammx_status() != rc1) {
        printf("FAIL: amiport_ammx_status() != amiport_ammx_init() result\n");
        return 10;
    }

    printf("Second call cache check: PASS\n");
    return 0;
}
