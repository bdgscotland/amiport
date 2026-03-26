/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* amiport: removed <sys/cdefs.h> -- not available in libnix.
 * Define __BEGIN_DECLS/__END_DECLS as C89-compatible empty macros. */
#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS
#define __END_DECLS
#endif

/* amiport: uint32_t -- sys/types.h includes sys/_stdint.h which defines it.
 * Include sys/types.h here to pull in uint32_t before our declarations.
 * Do NOT redefine it -- the system header already provides it. */
#include <sys/types.h>

/* amiport: off_t provided by <amiport/unistd.h> -- include it here so
 * all translation units that include extern.h get the type. */
#include <amiport/unistd.h>

__BEGIN_DECLS
int	crc(int, uint32_t *, off_t *);
void	pcrc(char *, uint32_t, off_t);
void	psum1(char *, uint32_t, off_t);
void	psum2(char *, uint32_t, off_t);
int	csum1(int, uint32_t *, off_t *);
int	csum2(int, uint32_t *, off_t *);
int	crc32(int, uint32_t *, off_t *);
__END_DECLS
