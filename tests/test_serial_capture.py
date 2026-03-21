"""Tests for scripts/serial-capture.py — TCP client for Enforcer serial capture.

serial-capture.py CONNECTS to FS-UAE's serial TCP port (FS-UAE is the server).

Covers test matrix items T1-T3 from docs/designs/autonomous-debug-agent.md.
"""

import os
import signal
import socket
import subprocess
import sys
import tempfile
import time

import pytest

SCRIPT = os.path.join(
    os.path.dirname(__file__), "..", "scripts", "serial-capture.py"
)


def _start_server(port):
    """Start a TCP server on the given port (simulating FS-UAE)."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("127.0.0.1", port))
    s.listen(1)
    return s


def _find_free_port():
    """Find a free ephemeral port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class TestT1ConnectAndReady:
    """T1: serial-capture.py connects and writes ready file."""

    def test_connects_and_writes_ready_file(self):
        port = _find_free_port()
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "enforcer.log")
            ready_path = os.path.join(tmpdir, ".ready")

            server = _start_server(port)
            try:
                proc = subprocess.Popen(
                    [
                        sys.executable, SCRIPT,
                        f"127.0.0.1:{port}", log_path,
                        "--ready-file", ready_path,
                    ],
                    stderr=subprocess.PIPE,
                )

                server.settimeout(5)
                client, _ = server.accept()

                deadline = time.monotonic() + 3
                while not os.path.exists(ready_path):
                    assert time.monotonic() < deadline, "Ready file not created"
                    time.sleep(0.05)

                assert os.path.isfile(ready_path)
                assert open(ready_path).read() == "connected"

                client.close()
                proc.wait(timeout=3)
                assert proc.returncode == 0
            finally:
                server.close()


class TestT2DataCapture:
    """T2: serial-capture.py captures serial data to log file."""

    def test_captures_exact_bytes(self):
        port = _find_free_port()
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "enforcer.log")

            server = _start_server(port)
            try:
                proc = subprocess.Popen(
                    [sys.executable, SCRIPT, f"127.0.0.1:{port}", log_path],
                    stderr=subprocess.PIPE,
                )
                server.settimeout(5)
                client, _ = server.accept()

                test_data = b"LONG-READ from 00000014  PC: 00343F4A\n"
                client.sendall(test_data)
                time.sleep(0.2)
                client.close()

                proc.wait(timeout=3)
                assert proc.returncode == 0

                with open(log_path, "rb") as f:
                    captured = f.read()
                assert captured == test_data
            finally:
                server.close()

    def test_captures_large_payload(self):
        port = _find_free_port()
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "enforcer.log")

            server = _start_server(port)
            try:
                proc = subprocess.Popen(
                    [sys.executable, SCRIPT, f"127.0.0.1:{port}", log_path],
                    stderr=subprocess.PIPE,
                )
                server.settimeout(5)
                client, _ = server.accept()

                test_data = b"X" * 8192
                client.sendall(test_data)
                time.sleep(0.3)
                client.close()

                proc.wait(timeout=3)
                with open(log_path, "rb") as f:
                    captured = f.read()
                assert len(captured) == 8192
            finally:
                server.close()


class TestT3ConnectionTimeout:
    """T3: serial-capture.py exits with code 2 when it can't connect."""

    def test_timeout_no_server(self):
        port = _find_free_port()
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "enforcer.log")

            wrapper = os.path.join(tmpdir, "wrapper.py")
            with open(wrapper, "w") as f:
                f.write(f"""
import sys, importlib
sys.path.insert(0, '{os.path.dirname(SCRIPT)}')
mod = importlib.import_module('serial-capture')
mod.CONNECT_TIMEOUT_SECS = 1
mod.CONNECT_RETRY_INTERVAL = 0.2
mod.main()
""")
            proc = subprocess.Popen(
                [sys.executable, wrapper, f"127.0.0.1:{port}", log_path],
                stderr=subprocess.PIPE,
            )
            proc.wait(timeout=10)
            assert proc.returncode == 2


class TestUsageError:
    def test_no_args(self):
        proc = subprocess.run(
            [sys.executable, SCRIPT], capture_output=True,
        )
        assert proc.returncode == 1

    def test_one_arg(self):
        proc = subprocess.run(
            [sys.executable, SCRIPT, "127.0.0.1:1234"],
            capture_output=True,
        )
        assert proc.returncode == 1


class TestSignalHandling:
    def test_sigterm_during_connection(self):
        port = _find_free_port()
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = os.path.join(tmpdir, "enforcer.log")

            server = _start_server(port)
            try:
                proc = subprocess.Popen(
                    [sys.executable, SCRIPT, f"127.0.0.1:{port}", log_path],
                    stderr=subprocess.PIPE,
                )
                server.settimeout(5)
                client, _ = server.accept()

                client.sendall(b"partial data\n")
                time.sleep(0.2)

                proc.send_signal(signal.SIGTERM)
                proc.wait(timeout=3)
                assert proc.returncode == 1

                if os.path.exists(log_path):
                    with open(log_path, "rb") as f:
                        data = f.read()
                    assert b"partial" in data
            finally:
                client.close()
                server.close()
