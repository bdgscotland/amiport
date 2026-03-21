"""Tests for scripts/serial-capture.py — TCP daemon for Enforcer serial capture."""

import os
import signal
import socket
import subprocess
import sys
import tempfile
import time

SCRIPT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "scripts", "serial-capture.py"
)


def _wait_for_sentinel(sentinel_path, timeout=2.0):
    """Poll until sentinel file exists and return its contents."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if os.path.exists(sentinel_path):
            with open(sentinel_path, "r") as f:
                contents = f.read()
            if contents:
                return contents
        time.sleep(0.05)
    return None


def _start_capture(log_path, sentinel_path, extra_env=None):
    """Launch serial-capture.py as a subprocess and return the Popen handle."""
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)
    return subprocess.Popen(
        [sys.executable, SCRIPT_PATH, log_path, sentinel_path],
        stderr=subprocess.PIPE,
        env=env,
    )


class TestBindAndSentinel:
    """T1: Verify bind, sentinel creation, and port validity."""

    def test_sentinel_created_within_timeout(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None, "Sentinel file not created within 2s"
            finally:
                proc.terminate()
                proc.wait(timeout=3)

    def test_sentinel_contains_valid_port(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None, "Sentinel file not created"
                port = int(contents)
                assert port > 0, f"Port must be positive, got {port}"
                assert port <= 65535, f"Port must be <= 65535, got {port}"
            finally:
                proc.terminate()
                proc.wait(timeout=3)

    def test_port_is_actually_listening(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None, "Sentinel file not created"
                port = int(contents)

                # Verify we can actually connect
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                try:
                    sock.connect(("127.0.0.1", port))
                finally:
                    sock.close()
            finally:
                proc.terminate()
                proc.wait(timeout=3)

    def test_sentinel_written_atomically(self):
        """Sentinel is written via tmp+rename, so no partial reads."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None, "Sentinel file not created"
                # If written atomically, contents should be a complete integer
                port = int(contents)  # Should not raise ValueError
                assert port > 0
                # No .tmp file should remain
                assert not os.path.exists(sentinel_path + ".tmp")
            finally:
                proc.terminate()
                proc.wait(timeout=3)


class TestDataCapture:
    """T2: Verify data sent over TCP appears in the log file."""

    def test_captured_bytes_match(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None, "Sentinel file not created"
                port = int(contents)

                # Send fake Enforcer output
                test_data = (
                    b"ENFORCER HIT: Read from 00000004\n"
                    b"  By task 'test-prog' (0x12345678)\n"
                )
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                sock.connect(("127.0.0.1", port))
                sock.sendall(test_data)
                sock.close()

                # Wait for process to exit (EOF on socket -> clean shutdown)
                proc.wait(timeout=3)
                assert proc.returncode == 0

                with open(log_path, "rb") as f:
                    logged = f.read()
                assert logged == test_data
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    proc.wait(timeout=3)

    def test_large_payload(self):
        """Verify multi-chunk data is captured completely."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None
                port = int(contents)

                # Send more than RECV_BUFFER_SIZE (4096) bytes
                test_data = b"ENFORCER HIT: line\n" * 500  # ~9500 bytes
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                sock.connect(("127.0.0.1", port))
                sock.sendall(test_data)
                sock.close()

                proc.wait(timeout=3)
                assert proc.returncode == 0

                with open(log_path, "rb") as f:
                    logged = f.read()
                assert logged == test_data
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    proc.wait(timeout=3)


class TestConnectionTimeout:
    """T3: Verify exit code 2 when no connection arrives."""

    def test_exits_with_code_2_on_timeout(self):
        """Start capture with a very short accept timeout, never connect."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")

            # Patch the timeout to 1 second so the test runs fast.
            # We do this by writing a wrapper script that monkey-patches
            # the constant before calling main().
            wrapper_path = os.path.join(tmpdir, "wrapper.py")
            with open(wrapper_path, "w") as f:
                f.write(
                    "import importlib.util, sys\n"
                    f"spec = importlib.util.spec_from_file_location('sc', {SCRIPT_PATH!r})\n"
                    "mod = importlib.util.module_from_spec(spec)\n"
                    "spec.loader.exec_module(mod)\n"
                    "mod.ACCEPT_TIMEOUT_SECS = 1\n"
                    "sys.argv = ['serial-capture.py', sys.argv[1], sys.argv[2]]\n"
                    "mod.main()\n"
                )

            proc = subprocess.Popen(
                [sys.executable, wrapper_path, log_path, sentinel_path],
                stderr=subprocess.PIPE,
            )

            # Verify sentinel is created (daemon is listening) but do NOT connect
            contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
            assert contents is not None, "Sentinel file not created"

            # Wait for timeout exit
            proc.wait(timeout=5)
            assert proc.returncode == 2


class TestSecondConnectionRejected:
    """Verify daemon accepts only one connection and does not crash on a second."""

    def test_second_connection_refused(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)
            try:
                contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
                assert contents is not None
                port = int(contents)

                # First connection (accepted)
                sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock1.settimeout(2.0)
                sock1.connect(("127.0.0.1", port))

                # Small delay to let the daemon close its listening socket
                time.sleep(0.2)

                # Second connection should be refused (listening socket closed)
                sock2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock2.settimeout(1.0)
                refused = False
                try:
                    sock2.connect(("127.0.0.1", port))
                except (ConnectionRefusedError, OSError):
                    refused = True
                finally:
                    sock2.close()

                assert refused, "Second connection should have been refused"

                # Clean up first connection so daemon exits
                sock1.sendall(b"data\n")
                sock1.close()

                proc.wait(timeout=3)
                assert proc.returncode == 0
            finally:
                if proc.poll() is None:
                    proc.terminate()
                    proc.wait(timeout=3)


class TestSignalHandling:
    """Verify clean shutdown on SIGTERM."""

    def test_sigterm_clean_shutdown(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)

            contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
            assert contents is not None, "Sentinel file not created"

            # Send SIGTERM
            proc.send_signal(signal.SIGTERM)
            proc.wait(timeout=3)

            # _cleanup on signal calls sys.exit(1)
            assert proc.returncode == 1

            # Sentinel should be cleaned up by _cleanup
            assert not os.path.exists(sentinel_path)

    def test_sigterm_during_connection(self):
        """SIGTERM while a client is connected should close cleanly."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "capture.log")
            sentinel_path = os.path.join(tmpdir, ".serial-ready")
            proc = _start_capture(log_path, sentinel_path)

            contents = _wait_for_sentinel(sentinel_path, timeout=2.0)
            assert contents is not None
            port = int(contents)

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2.0)
            sock.connect(("127.0.0.1", port))
            sock.sendall(b"partial data\n")

            # Small delay so the daemon processes the data
            time.sleep(0.1)

            proc.send_signal(signal.SIGTERM)
            proc.wait(timeout=3)

            sock.close()

            # Log should contain the data sent before SIGTERM
            if os.path.exists(log_path):
                with open(log_path, "rb") as f:
                    logged = f.read()
                assert logged == b"partial data\n"


class TestUsageError:
    """Verify exit code 1 on bad arguments."""

    def test_no_args(self):
        proc = subprocess.Popen(
            [sys.executable, SCRIPT_PATH],
            stderr=subprocess.PIPE,
        )
        proc.wait(timeout=3)
        assert proc.returncode == 1

    def test_one_arg(self):
        proc = subprocess.Popen(
            [sys.executable, SCRIPT_PATH, "/tmp/log"],
            stderr=subprocess.PIPE,
        )
        proc.wait(timeout=3)
        assert proc.returncode == 1
