# lib/libcurl/Makefile
# libcurl 8.11.1 HTTP-only build for the NetSurf-Vampire dep stack.
#
# Convention matches lib/libpng and lib/libjpeg (NetSurf-Vampire Phase D-prime).
# Whole-archive -O0 default per crash-patterns #16. Stage 7 perf-optimizer can
# promote individual hot files to -O1 after struct-return audit.
#
# CRITICAL: NDK ships <sys/socket.h> + <netdb.h> + <netinet/in.h>. We rely on
# those (mapped to bsdsocket.library natively by NDK) instead of the
# bsdsocket-shim (lib/bsdsocket-shim/include/amiport-net/...). Consumers that
# need the shim's WaitSelect-with-mixed-fds / fd 0/1/2 collision avoidance /
# getaddrinfo emulation should link both libraries. libcurl's runtime path
# uses bsdsocket directly; advanced features live in the shim.

TOOLCHAIN_BIN = ../../toolchain/scripts
CC = $(TOOLCHAIN_BIN)/m68k-amigaos-gcc
AR = $(TOOLCHAIN_BIN)/m68k-amigaos-ar
RANLIB = $(TOOLCHAIN_BIN)/m68k-amigaos-ranlib

CFLAGS = -O0 -fno-strict-aliasing \
         -m68040 -m68881 \
         -noixemul \
         -DNDEBUG -D_DEFAULT_SOURCE \
         -DBUILDING_LIBCURL \
         -DCURL_STATICLIB \
         -std=c99 \
         -Iinclude -Isrc \
         -Wno-pointer-sign -Wno-unused-parameter

# HTTP-only source list. Initial best-guess subset of LIB_CFILES from upstream
# Makefile.inc; expand on linker-error iteration. Excludes:
#   - All vauth/ vtls/ vquic/ vssh/ subdirs (TLS, auth, QUIC, SSH not vendored)
#   - Protocol files: dict/file/ftp/gopher/imap/ldap/mqtt/pop3/rtsp/smb/smtp/telnet/tftp
#   - Auth: krb5, http_aws_sigv4, http_digest, http_negotiate, http_ntlm,
#     curl_des, curl_endian, curl_gssapi, curl_ntlm_core, curl_sasl,
#     curl_sha512_256, curl_sspi, hmac, md4, sha256
#   - Proxy: cf-h1-proxy, cf-h2-proxy, cf-haproxy, http_proxy, noproxy, socks*
#   - Cookies: cookie, parsedate, psl
#   - HTTP/2/QUIC: c-hyper, http2, cf-https-connect
#   - Async DNS: asyn-ares, asyn-thread, hostasyn, hostip6
#   - WebSockets: ws
#   - Form API: formdata, fileinfo, fopen, mime
#   - HSTS / Alt-Svc: altsvc, hsts
#   - DOH: doh
#   - netrc: netrc
#   - Win32 / debug: dllmain, system_win32, version_win32, memdebug
#   - macos: macos
#   - idn: idn (no libidn)
#   - openldap: openldap
#   - rtmp: curl_rtmp
#   - file/ftp wildcard: curl_fnmatch, curl_range
#   - ftp pingpong: pingpong, ftplistparser
#   - rename helper (cookie jar): rename
#   - share interface (NetSurf does not use): share

SRCS = \
	src/amigaos.c \
	src/base64.c \
	src/bufq.c \
	src/bufref.c \
	src/cf-https-connect.c \
	src/cf-socket.c \
	src/cfilters.c \
	src/conncache.c \
	src/connect.c \
	src/content_encoding.c \
	src/curl_addrinfo.c \
	src/curl_get_line.c \
	src/curl_gethostname.c \
	src/curl_memrchr.c \
	src/curl_multibyte.c \
	src/curl_sha512_256.c \
	src/curl_threads.c \
	src/curl_trc.c \
	src/cw-out.c \
	src/dynbuf.c \
	src/dynhds.c \
	src/easy.c \
	src/easygetopt.c \
	src/easyoptions.c \
	src/escape.c \
	src/getenv.c \
	src/getinfo.c \
	src/hash.c \
	src/headers.c \
	src/hostip.c \
	src/hostip4.c \
	src/hostsyn.c \
	src/http.c \
	src/http1.c \
	src/http_chunks.c \
	src/http_digest.c \
	src/idn.c \
	src/if2ip.c \
	src/inet_ntop.c \
	src/inet_pton.c \
	src/llist.c \
	src/md5.c \
	src/mprintf.c \
	src/multi.c \
	src/nonblock.c \
	src/parsedate.c \
	src/progress.c \
	src/rand.c \
	src/request.c \
	src/select.c \
	src/sendf.c \
	src/setopt.c \
	src/sha256.c \
	src/share.c \
	src/slist.c \
	src/vauth/digest.c \
	src/vauth/vauth.c \
	src/vquic/vquic.c \
	src/vtls/vtls.c \
	src/speedcheck.c \
	src/splay.c \
	src/strcase.c \
	src/strdup.c \
	src/strerror.c \
	src/strtok.c \
	src/strtoofft.c \
	src/timediff.c \
	src/timeval.c \
	src/transfer.c \
	src/url.c \
	src/urlapi.c \
	src/version.c \
	src/warnless.c

OBJS = $(SRCS:.c=.o)

all: libcurl.a

libcurl.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libcurl.a

.PHONY: all clean
