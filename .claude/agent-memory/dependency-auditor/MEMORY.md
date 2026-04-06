# Memory Index — Dependency Auditor Agent

- [jq-1.7.1-audit.md](jq-1.7.1-audit.md) - Full dependency audit for jq 1.7.1: oniguruma optional, pthreads need single-threaded stub shim, libm/libnix available, decNumber bundled
- [wget-1.20.3-audit.md](wget-1.20.3-audit.md) - Full dependency audit for wget 1.20.3: AmiSSL for HTTPS, bsdsocket-shim gaps (select/read/write/close on socket fds), POSIX regex must be bundled, iconv/zlib/NTLM/fork optional
