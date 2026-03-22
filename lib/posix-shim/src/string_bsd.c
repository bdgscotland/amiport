/*
 * string_bsd.c — BSD string utility implementations for AmigaOS
 *
 * Pure C implementations of strlcpy, strlcat, reallocarray, strcasecmp,
 * strncasecmp, and strcasestr.
 * No AmigaOS-specific calls needed — these are portable C.
 */

#include <amiport/string.h>

#include <ctype.h>
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

void *
amiport_recallocarray(void *ptr, size_t oldnmemb, size_t nmemb, size_t size)
{
    size_t oldsize, newsize;
    void *newptr;

    /* Check for multiplication overflow on new size */
    if (size != 0 && nmemb > (size_t)-1 / size) {
        errno = ENOMEM;
        return NULL;
    }
    newsize = nmemb * size;

    /* Check for multiplication overflow on old size */
    if (size != 0 && oldnmemb > (size_t)-1 / size) {
        errno = ENOMEM;
        return NULL;
    }
    oldsize = oldnmemb * size;

    /* If shrinking or same size, just realloc */
    if (newsize <= oldsize)
        return realloc(ptr, newsize);

    /* Growing: allocate new block, copy old data, zero the rest */
    newptr = realloc(ptr, newsize);
    if (newptr == NULL)
        return NULL;

    memset((char *)newptr + oldsize, 0, newsize - oldsize);
    return newptr;
}

int
amiport_strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;

    for (;;) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2)
            return (int)c1 - (int)c2;
        if (c1 == '\0')
            return 0;
        s1++;
        s2++;
    }
}

int
amiport_strncasecmp(const char *s1, const char *s2, size_t n)
{
    unsigned char c1, c2;

    if (n == 0)
        return 0;

    for (;;) {
        c1 = (unsigned char)tolower((unsigned char)*s1);
        c2 = (unsigned char)tolower((unsigned char)*s2);
        if (c1 != c2)
            return (int)c1 - (int)c2;
        if (c1 == '\0' || --n == 0)
            return 0;
        s1++;
        s2++;
    }
}

char *
amiport_strcasestr(const char *haystack, const char *needle)
{
    size_t needle_len;
    size_t i;
    unsigned char nh, nc;

    if (*needle == '\0')
        return (char *)haystack;

    needle_len = strlen(needle);

    while (*haystack != '\0') {
        /* Quick check: first char must match before calling strncasecmp */
        nh = (unsigned char)tolower((unsigned char)*haystack);
        nc = (unsigned char)tolower((unsigned char)*needle);
        if (nh == nc) {
            /* Compare the rest of needle against haystack */
            for (i = 1; i < needle_len; i++) {
                if (haystack[i] == '\0')
                    return NULL;
                if (tolower((unsigned char)haystack[i]) !=
                    tolower((unsigned char)needle[i]))
                    break;
            }
            if (i == needle_len)
                return (char *)haystack;
        }
        haystack++;
    }

    return NULL;
}

/*
 * explicit_bzero — secure zero-fill that the compiler cannot optimize away
 *
 * Uses a volatile function pointer to force the memset call to be emitted
 * even in optimized builds. This is the standard technique for clearing
 * sensitive data (keys, passwords) before freeing the buffer.
 *
 * amiport: direct mapping — OpenBSD/glibc explicit_bzero semantics preserved.
 */
static void
amiport_explicit_bzero_fill(void *s, size_t n)
{
    memset(s, 0, n);
}

static void (* volatile amiport_explicit_bzero_fn)(void *, size_t) =
    amiport_explicit_bzero_fill;

void
amiport_explicit_bzero(void *s, size_t n)
{
    if (n == 0)
        return;
    amiport_explicit_bzero_fn(s, n);
}
