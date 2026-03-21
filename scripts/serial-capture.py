#!/usr/bin/env python3
"""serial-capture.py — Capture Amiga Enforcer serial output via TCP.

FS-UAE can redirect the Amiga serial port to a TCP connection. This daemon
binds an ephemeral TCP port, accepts exactly one connection from FS-UAE's
serial emulation, and streams all received data to a log file.

# Lifecycle:
#   bind(0) → write sentinel(port) → accept(1 conn) → stream to file → EOF → exit

Usage:
    python3 scripts/serial-capture.py <log-file> <sentinel-file>

Arguments:
    log-file       Path to write captured serial data (e.g., enforcer.log)
    sentinel-file  Path to write the assigned TCP port number (e.g., .serial-ready)

The sentinel file allows the caller (test-fsemu.sh) to discover the actual
port number after OS assignment. The caller polls for this file to know when
the listener is ready before launching FS-UAE.

Exit codes:
    0   Clean shutdown (connection closed by FS-UAE)
    1   Usage error or fatal failure
    2   Connection timeout (no connection within 30 seconds)
"""

import os
import signal
import socket
import sys

# -- Constants ---------------------------------------------------------------

ACCEPT_TIMEOUT_SECS = 30
RECV_BUFFER_SIZE = 4096

# -- Global state for signal handler cleanup ---------------------------------

_server_sock = None
_client_sock = None
_log_fp = None
_sentinel_path = None


def _cleanup(signum=None, frame=None):
    """Close sockets, flush and close log, remove sentinel, then exit."""
    if _client_sock:
        try:
            _client_sock.close()
        except OSError:
            pass
    if _server_sock:
        try:
            _server_sock.close()
        except OSError:
            pass
    if _log_fp and not _log_fp.closed:
        try:
            _log_fp.flush()
            _log_fp.close()
        except OSError:
            pass
    if _sentinel_path:
        try:
            os.unlink(_sentinel_path)
        except OSError:
            pass
    if signum is not None:
        sys.exit(1)


def main():
    global _server_sock, _client_sock, _log_fp, _sentinel_path

    if len(sys.argv) != 3:
        print(
            "Usage: serial-capture.py <log-file> <sentinel-file>",
            file=sys.stderr,
        )
        sys.exit(1)

    log_path = sys.argv[1]
    _sentinel_path = sys.argv[2]

    # Install signal handlers for graceful shutdown
    signal.signal(signal.SIGTERM, _cleanup)
    signal.signal(signal.SIGINT, _cleanup)

    # -- Bind ephemeral port -------------------------------------------------
    _server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    _server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    _server_sock.bind(("127.0.0.1", 0))
    _server_sock.listen(1)

    assigned_port = _server_sock.getsockname()[1]

    # -- Write sentinel file (atomic via temp + rename) ----------------------
    sentinel_tmp = _sentinel_path + ".tmp"
    try:
        with open(sentinel_tmp, "w") as f:
            f.write(str(assigned_port))
        os.rename(sentinel_tmp, _sentinel_path)
    except OSError as e:
        print(
            f"serial-capture: failed to write sentinel: {e}",
            file=sys.stderr,
        )
        _cleanup()
        sys.exit(1)

    print(
        f"serial-capture: listening on 127.0.0.1:{assigned_port}",
        file=sys.stderr,
    )

    # -- Accept exactly one connection (with timeout) ------------------------
    _server_sock.settimeout(ACCEPT_TIMEOUT_SECS)
    try:
        _client_sock, addr = _server_sock.accept()
    except socket.timeout:
        print(
            f"serial-capture: no connection within {ACCEPT_TIMEOUT_SECS}s, exiting",
            file=sys.stderr,
        )
        _cleanup()
        sys.exit(2)

    # No longer need the listening socket
    _server_sock.close()
    _server_sock = None

    print(
        f"serial-capture: connection from {addr[0]}:{addr[1]}",
        file=sys.stderr,
    )

    # -- Open log file -------------------------------------------------------
    try:
        _log_fp = open(log_path, "wb")
    except OSError as e:
        print(
            f"serial-capture: failed to open log file: {e}",
            file=sys.stderr,
        )
        _cleanup()
        sys.exit(1)

    # -- Stream data until EOF -----------------------------------------------
    try:
        while True:
            data = _client_sock.recv(RECV_BUFFER_SIZE)
            if not data:
                break
            try:
                _log_fp.write(data)
                _log_fp.flush()
            except OSError as e:
                print(
                    f"serial-capture: log write error: {e}",
                    file=sys.stderr,
                )
                break
    except ConnectionResetError:
        # FS-UAE closed the connection abruptly — not an error
        pass
    except OSError as e:
        print(
            f"serial-capture: recv error: {e}",
            file=sys.stderr,
        )

    # -- Clean shutdown ------------------------------------------------------
    _cleanup()
    print("serial-capture: done", file=sys.stderr)
    sys.exit(0)


if __name__ == "__main__":
    main()
