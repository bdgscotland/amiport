/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#ifndef DROPBEAR_INCLUDES_H_
#define DROPBEAR_INCLUDES_H_

#include "options.h"
#include "debug.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

/* amiport: socket.h must come AFTER fcntl.h because
 * amiport-net/socket.h defines a fcntl() macro */
#ifdef __AMIGA__
#include <amiport-net/socket.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifndef __AMIGA__
#include <sys/wait.h>
#include <sys/resource.h>
#endif
#ifndef __AMIGA__
#include <grp.h>
#endif
#include <limits.h>
#ifndef __AMIGA__
#include <pwd.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifndef __AMIGA__
#include <termios.h>
#endif
#include <unistd.h>
#ifndef __AMIGA__
#include <syslog.h>
#endif
#ifdef __AMIGA__
#include <amiport-net/netdb.h>
#else
#include <netdb.h>
#endif
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>
#include <setjmp.h>
#include <assert.h>

#ifdef __AMIGA__
/* amiport: AmigaOS stubs for missing POSIX functionality */
#include "amigaos_stubs.h"
#endif

#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef HAVE_LASTLOG_H
#include <lastlog.h>
#endif

#ifdef __AMIGA__
#include <amiport-net/netinet/in.h>
#include <amiport-net/arpa/inet.h>
#else
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#include <netinet/ip.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#endif /* __AMIGA__ */

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_UTIL_H
#include <util.h>
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_RANDOM_H
#include <sys/random.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#ifdef __AMIGA__
/* amiport: use standalone lib copies, not bundled.
 * -I flags point to lib/libtomcrypt/include and lib/libtommath/include */
#include <tomcrypt.h>
#include <tommath.h>
#else
#ifdef BUNDLED_LIBTOM
#include "../libtomcrypt/src/headers/tomcrypt.h"
#include "../libtommath/tommath.h"
#else
#include <tomcrypt.h>
#include <tommath.h>
#endif
#endif

#include "compat.h"

#ifndef HAVE_U_INT8_T
typedef unsigned char u_int8_t;
#endif /* HAVE_U_INT8_T */
#ifndef HAVE_UINT8_T
typedef u_int8_t uint8_t;
#endif /* HAVE_UINT8_T */

#ifndef HAVE_U_INT16_T
typedef unsigned short u_int16_t;
#endif /* HAVE_U_INT16_T */
#ifndef HAVE_UINT16_T
typedef u_int16_t uint16_t;
#endif /* HAVE_UINT16_T */

#ifndef HAVE_U_INT32_T
typedef unsigned int u_int32_t;
#endif /* HAVE_U_INT32_T */
#ifndef HAVE_UINT32_T
typedef u_int32_t uint32_t;
#endif /* HAVE_UINT32_T */

#ifndef SIZE_T_MAX
#define SIZE_T_MAX ULONG_MAX
#endif /* SIZE_T_MAX */

#ifdef HAVE_LINUX_PKT_SCHED_H
#include <linux/types.h>
#include <linux/pkt_sched.h>
#endif

#if DROPBEAR_PLUGIN
#include <dlfcn.h>
#endif

extern char** environ;

#include "fake-rfc2553.h"

/* amiport: fuzz.h removed -- not building fuzzer */
#ifndef DROPBEAR_FUZZ
#define DROPBEAR_FUZZ 0
#endif

#ifndef LOG_AUTHPRIV
#define LOG_AUTHPRIV LOG_AUTH
#endif

/* so we can avoid warnings about unused params (ie in signal handlers etc) */
#ifdef UNUSED 
#elif defined(__GNUC__) 
# define UNUSED(x) UNUSED_ ## x __attribute__((unused)) 
#elif defined(__LCLINT__) 
# define UNUSED(x) /*@unused@*/ x 
#else 
# define UNUSED(x) x 
#endif

/* static_assert() is a keyword in c23, earlier libc often supports
 * it as a macro in assert.h.
 * _Static_assert() is a keyword supported since c11.
 * If neither are available, do nothing */
#ifndef HAVE_STATIC_ASSERT
#ifdef HAVE_UNDERSCORE_STATIC_ASSERT
#define static_assert(condition, message) _Static_assert(condition, message)
#else
#define static_assert(condition, message)
#endif
#endif

#endif /* DROPBEAR_INCLUDES_H_ */
