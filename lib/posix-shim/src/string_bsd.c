/*
 * string_bsd.c — BSD string utility implementations for AmigaOS
 *
 * Pure C implementations of strlcpy, strlcat, and reallocarray.
 * No AmigaOS-specific calls needed — these are portable C.
 */

#include <amiport/string.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

size_t
amiport_strlcpy(char *dst, const char *src, size_t dst_size)
{
    size_t src_len = strlen(src);

    if (dst_size > 0) {
        size_t copy_len = (src_len >= dst_size) ? dst_size - 1 : src_len;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }

    return src_len;
}

size_t
amiport_strlcat(char *dst, const char *src, size_t dst_size)
{
    size_t dst_len;
    size_t src_len = strlen(src);
    size_t i;

    /* Find existing NUL, but don't scan past dst_size */
    for (i = 0; i < dst_size; i++) {
        if (dst[i] == '\0')
            break;
    }
    dst_len = i;

    /* dst not NUL-terminated within dst_size */
    if (dst_len == dst_size)
        return dst_size + src_len;

    if (dst_len + src_len < dst_size) {
        memcpy(dst + dst_len, src, src_len + 1);
    } else {
        memcpy(dst + dst_len, src, dst_size - dst_len - 1);
        dst[dst_size - 1] = '\0';
    }

    return dst_len + src_len;
}

void *
amiport_reallocarray(void *ptr, size_t nmemb, size_t size)
{
    /* Check for multiplication overflow */
    if (size != 0 && nmemb > (size_t)-1 / size) {
        errno = ENOMEM;
        return NULL;
    }

    return realloc(ptr, nmemb * size);
}
