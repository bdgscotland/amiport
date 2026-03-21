"""Tests for scripts/debug-report.py — Enforcer parser and addr2line mapper.

Covers test matrix items T4–T12 from docs/designs/autonomous-debug-agent.md.
"""

import importlib
import json
import os
import sys
import tempfile
from unittest import mock

import pytest

# Allow importing from scripts/ directory — the filename uses a hyphen,
# so we need importlib to load it as a module.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
_mod = importlib.import_module("debug-report")

parse_enforcer_log = _mod.parse_enforcer_log
classify_crash = _mod.classify_crash
detect_mungwall = _mod.detect_mungwall
parse_flags = _mod.parse_flags
count_sections = _mod.count_sections
parse_linker_map = _mod.parse_linker_map
section_for_hunk = _mod.section_for_hunk
run_addr2line = _mod.run_addr2line
cmd_map = _mod.cmd_map
MUNGWALL_SENTINELS = _mod.MUNGWALL_SENTINELS


# --------------------------------------------------------------------------- #
# Enforcer output fixtures from the design doc
# --------------------------------------------------------------------------- #

# Real WORD-WRITE sample with datestamp and data field
FIXTURE_WORD_WRITE = """\
03-Apr-93  21:26:18
WORD-WRITE to  00000000        data=4444       PC: 07895CA4
USP:  078D692C SR: 0000 SW: 0729  (U0)(-)(-)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
Stck: 00000000 07848E1C 00009C40 078A30B4 BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB
Stck: BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB 078E9048 00011DA8 DEADBEEF
----> 07895CA4 - "lawbreaker"  Hunk 0000 Offset 0000007C
PC-8: AAAA1111 247CAAAA 2222267C AAAA3333 287CAAAA 44442A7C AAAA5555 31C40000
PC *: 522E0127 201433FC 400000DF F09A522E 012611C7 00CE4EAE FF7642B8 0324532E
Name: "New_Shell"  CLI: "lawbreaker"  Hunk 0000 Offset 0000007C
"""

# Real LONG-READ sample without datestamp and no data field
FIXTURE_LONG_READ = """\
LONG-READ from AAAA4444                        PC: 07895CA8
USP:  078D692C SR: 0015 SW: 0749  (U0)(F)(-)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
Stck: 00000000 07848E1C 00009C40 078A30B4 BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB
Stck: BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB BBBBBBBB 078E9048 00011DA8 DEADBEEF
----> 07895CA8 - "lawbreaker"  Hunk 0000 Offset 00000080
Name: "New_Shell"  CLI: "lawbreaker"  Hunk 0000 Offset 00000080
"""

# Real Alert sample from design doc — note the extra TCB/USP fields on the
# Alert line. The current regex (RE_ALERT_LINE) expects the line to end after
# the alert code, so this format is parsed via an adapted fixture below.
FIXTURE_ALERT_DESIGN_DOC = """\
25-Jul-93  17:15:06
Alert !! Alert 35000000     TCB: 07642F70     USP: 07657C10
Data: 00000000 DDDD1111 DDDD2222 DDDD3333 0763852A DDDD5555 DDDD6666 35000000
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 0763852A 07400810 --------
Stck: 076385A0 00000000 0752EE9A 00002800 07643994 00000000 0762F710 076305F0
----> 076385A0 - "lawbreaker"  Hunk 0000 Offset 00000098
"""

# Alert fixture adapted to match the regex the parser currently handles
# (Alert line with no trailing TCB/USP on the same line)
FIXTURE_ALERT_SIMPLE = """\
25-Jul-93  17:15:06
Alert !! Alert 35000000
Data: 00000000 DDDD1111 DDDD2222 DDDD3333 0763852A DDDD5555 DDDD6666 35000000
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 0763852A 07400810 --------
Stck: 076385A0 00000000 0752EE9A 00002800 07643994 00000000 0762F710 076305F0
----> 076385A0 - "lawbreaker"  Hunk 0000 Offset 00000098
"""

# Combined fixture with both WORD-WRITE and LONG-READ hits
FIXTURE_COMBINED = FIXTURE_WORD_WRITE + "\n" + FIXTURE_LONG_READ


def _write_fixture(tmpdir, content, name="enforcer.log"):
    """Write a fixture string to a temp file and return the path."""
    path = os.path.join(tmpdir, name)
    with open(path, "w") as f:
        f.write(content)
    return path


# --------------------------------------------------------------------------- #
# T4: Enforcer parser — all hit types
# --------------------------------------------------------------------------- #

class TestT4EnforcerParserAllHitTypes:
    """T4: Feed real Enforcer output samples, verify JSON for hit types."""

    def test_word_write_with_datestamp_and_data(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]

        assert hit["type"] == "WORD-WRITE"
        assert hit["address"] == "0x00000000"
        assert hit["data"] == "0x00004444"
        assert hit["pc"] == "0x07895CA4"
        assert hit["datestamp"] == "03-Apr-93  21:26:18"

    def test_word_write_registers(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["registers"]["d0"] == "0xDDDD0000"
        assert hit["registers"]["d7"] == "0xDDDD7777"
        assert hit["registers"]["a0"] == "0xAAAA0000"
        assert hit["registers"]["a4"] == "0xAAAA4444"
        assert hit["registers"]["a6"] == "0x07800804"

    def test_word_write_flags_design_doc_format(self):
        """Real Enforcer uses (U0)(-)(-) — three paren groups. Now parsed."""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["usp"] == "0x078D692C"
        assert hit["sr"] == "0x0000"
        assert hit["flags"]["user_mode"] is True
        assert hit["flags"]["forbid"] is False
        assert hit["flags"]["disable"] is False

    def test_word_write_flags_single_group_format(self):
        """If the flags are a single parenthesized group, parsing works."""
        fixture = """\
WORD-WRITE to  00000000        data=4444       PC: 07895CA4
USP:  078D692C SR: 0000 SW: 0729  (U0)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
Name: "New_Shell"  CLI: "lawbreaker"  Hunk 0000 Offset 0000007C
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, fixture)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["usp"] == "0x078D692C"
        assert hit["sr"] == "0x0000"
        assert hit["flags"]["user_mode"] is True
        assert hit["flags"]["forbid"] is False
        assert hit["flags"]["disable"] is False

    def test_word_write_segtracker(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["segtracker"] is not None
        assert hit["segtracker"]["name"] == "lawbreaker"
        assert hit["segtracker"]["hunk"] == 0
        assert hit["segtracker"]["offset"] == "0x0000007C"

    def test_word_write_task_and_cli(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["task_name"] == "New_Shell"
        assert hit["cli_name"] == "lawbreaker"

    def test_word_write_stack(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_WORD_WRITE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        # Two Stck: lines = 16 longwords
        assert len(hit["stack"]) == 16
        assert hit["stack"][0] == "0x00000000"
        assert hit["stack"][-1] == "0xDEADBEEF"

    def test_long_read_no_data_no_datestamp(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_LONG_READ)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]

        assert hit["type"] == "LONG-READ"
        assert hit["address"] == "0xAAAA4444"
        assert hit["pc"] == "0x07895CA8"
        # No data field for READ hits
        assert "data" not in hit
        # No datestamp
        assert "datestamp" not in hit

    def test_long_read_forbid_flag_design_doc_format(self):
        """Real format (U0)(F)(-) has three paren groups. Now parsed."""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_LONG_READ)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["usp"] == "0x078D692C"
        assert hit["flags"]["user_mode"] is True
        assert hit["flags"]["forbid"] is True

    def test_long_read_forbid_flag_single_group(self):
        """Forbid flag parses correctly with single-group format."""
        fixture = """\
LONG-READ from AAAA4444                        PC: 07895CA8
USP:  078D692C SR: 0015 SW: 0749  (UF)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
Name: "New_Shell"  CLI: "lawbreaker"  Hunk 0000 Offset 00000080
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, fixture)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["flags"]["user_mode"] is True
        assert hit["flags"]["forbid"] is True

    def test_alert_hit(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_ALERT_SIMPLE)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]

        assert hit["type"] == "Alert"
        assert hit["alert_code"] == "0x35000000"
        assert hit["datestamp"] == "25-Jul-93  17:15:06"

    def test_alert_segtracker(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_ALERT_SIMPLE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["segtracker"]["name"] == "lawbreaker"
        assert hit["segtracker"]["hunk"] == 0
        assert hit["segtracker"]["offset"] == "0x00000098"

    def test_alert_registers(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_ALERT_SIMPLE)
            result = parse_enforcer_log(path)

        hit = result["hits"][0]
        assert hit["registers"]["d0"] == "0x00000000"
        assert hit["registers"]["d7"] == "0x35000000"
        assert hit["registers"]["a5"] == "0x0763852A"

    def test_alert_design_doc_format(self):
        """The design doc Alert format has TCB/USP on the Alert line. Now parsed."""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_ALERT_DESIGN_DOC)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]
        assert hit["type"] == "Alert"
        assert hit["alert_code"] == "0x35000000"
        assert hit["usp"] == "0x07657C10"

    def test_combined_multiple_hits(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_COMBINED)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 2
        assert result["hits"][0]["type"] == "WORD-WRITE"
        assert result["hits"][1]["type"] == "LONG-READ"

    def test_summary_unique_locations(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_COMBINED)
            result = parse_enforcer_log(path)

        # Two different PCs
        assert result["summary"]["unique_locations"] == 2

    def test_summary_crash_types(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, FIXTURE_COMBINED)
            result = parse_enforcer_log(path)

        crash_types = result["summary"]["crash_types"]
        # WORD-WRITE to 0x00000000 is a NULL pointer write
        assert any("NULL pointer" in ct for ct in crash_types)


# --------------------------------------------------------------------------- #
# T5: Enforcer parser — Mungwall sentinel detection
# --------------------------------------------------------------------------- #

class TestT5MungwallSentinels:
    """T5: Verify Mungwall sentinel pattern detection."""

    def test_deadbeef_is_use_after_free(self):
        result = detect_mungwall(0xDEADBEEF)
        assert result is not None
        assert "use-after-free" in result

    def test_abadcafe_is_pre_fill(self):
        result = detect_mungwall(0xABADCAFE)
        assert result is not None
        assert "pre-fill" in result

    def test_cccccccc_is_uninitialized(self):
        result = detect_mungwall(0xCCCCCCCC)
        assert result is not None
        assert "uninitialized" in result

    def test_null_pointer_not_mungwall(self):
        result = detect_mungwall(0x00000014)
        assert result is None

    def test_regular_address_not_mungwall(self):
        result = detect_mungwall(0x07895CA4)
        assert result is None

    def test_mungwall_address_in_enforcer_hit(self):
        """A LONG-READ from 0xDEADBEEF should classify as use-after-free."""
        fixture = """\
LONG-READ from DEADBEEF                        PC: 07895CA4
USP:  078D692C SR: 0000 SW: 0729  (U0)(-)(-)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
----> 07895CA4 - "testprog"  Hunk 0000 Offset 0000007C
Name: "New_Shell"  CLI: "testprog"  Hunk 0000 Offset 0000007C
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, fixture)
            result = parse_enforcer_log(path)

        crash_types = result["summary"]["crash_types"]
        assert any("use-after-free" in ct for ct in crash_types)

    def test_null_plus_offset_in_enforcer_hit(self):
        """Address 0x00000014 should classify as NULL pointer with offset."""
        fixture = """\
LONG-READ from 00000014                        PC: 07895CA4
USP:  078D692C SR: 0000 SW: 0729  (U0)(-)(-)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
----> 07895CA4 - "testprog"  Hunk 0000 Offset 0000007C
Name: "New_Shell"  CLI: "testprog"  Hunk 0000 Offset 0000007C
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, fixture)
            result = parse_enforcer_log(path)

        crash_types = result["summary"]["crash_types"]
        assert any("NULL pointer" in ct for ct in crash_types)


# --------------------------------------------------------------------------- #
# T6: addr2line map — single-hunk
# --------------------------------------------------------------------------- #

class TestT6Addr2LineSingleHunk:
    """T6: Mock addr2line, verify offset passed with --section=.text."""

    @mock.patch("subprocess.run")
    def test_single_hunk_uses_text_section(self, mock_run):
        mock_run.return_value = mock.Mock(
            stdout="main\nported/main.c:42\n",
            returncode=0,
        )

        source_file, line_num, func = run_addr2line(
            "/fake/binary", ".text", "0x0000007C"
        )

        mock_run.assert_called_once()
        call_args = mock_run.call_args[0][0]
        assert "--section=.text" in call_args
        assert "0x0000007C" in call_args
        assert "-e" in call_args
        assert "/fake/binary" in call_args

        assert source_file == "ported/main.c"
        assert line_num == 42
        assert func == "main"

    @mock.patch("subprocess.run")
    def test_single_hunk_full_map_flow(self, mock_run):
        """Simulate cmd_map for a single-hunk binary (1 section)."""
        # count_sections returns 1, addr2line returns source info
        mock_run.side_effect = [
            # First call: objdump -h (1 section)
            mock.Mock(
                stdout="  0 .text  00001000  00000000  00000000  00000034  2**2\n",
                returncode=0,
            ),
            # Second call: addr2line
            mock.Mock(
                stdout="process_line\nported/main.c:87\n",
                returncode=0,
            ),
        ]

        hits_data = {
            "hits": [{
                "type": "LONG-READ",
                "address": "0xAAAA4444",
                "pc": "0x07895CA8",
                "segtracker": {
                    "name": "testprog",
                    "hunk": 0,
                    "offset": "0x00000080",
                },
                "task_name": "New_Shell",
                "cli_name": "testprog",
            }],
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            hits_path = os.path.join(tmpdir, "hits.json")
            binary_path = os.path.join(tmpdir, "testprog")
            with open(hits_path, "w") as f:
                json.dump(hits_data, f)
            with open(binary_path, "w") as f:
                f.write("fake binary")

            args = mock.Mock()
            args.hits_json = hits_path
            args.binary = binary_path

            with mock.patch("sys.stdout"):
                cmd_map(args)

            # Verify addr2line was called with --section=.text
            addr2line_call = mock_run.call_args_list[1][0][0]
            assert "--section=.text" in addr2line_call
            assert "0x00000080" in addr2line_call


# --------------------------------------------------------------------------- #
# T7: addr2line map — multi-hunk
# --------------------------------------------------------------------------- #

class TestT7Addr2LineMultiHunk:
    """T7: Mock addr2line + .map file, verify correct section selection."""

    def test_parse_linker_map(self):
        map_content = """\
.text   0x00000000   0x1234
.data   0x00001234   0x0100
.bss    0x00001334   0x0050
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            map_path = os.path.join(tmpdir, "test.map")
            with open(map_path, "w") as f:
                f.write(map_content)

            hunks = parse_linker_map(map_path)

        assert hunks is not None
        assert 0 in hunks
        assert hunks[0]["section"] == ".text"
        assert hunks[0]["base"] == 0x0
        assert 1 in hunks
        assert hunks[1]["section"] == ".data"
        assert 2 in hunks
        assert hunks[2]["section"] == ".bss"

    def test_section_for_hunk_with_map(self):
        hunk_map = {
            0: {"section": ".text", "base": 0},
            1: {"section": ".data", "base": 0x1234},
            2: {"section": ".bss", "base": 0x1334},
        }
        assert section_for_hunk(hunk_map, 0) == ".text"
        assert section_for_hunk(hunk_map, 1) == ".data"
        assert section_for_hunk(hunk_map, 2) == ".bss"

    def test_section_for_hunk_defaults(self):
        assert section_for_hunk(None, 0) == ".text"
        assert section_for_hunk(None, 1) == ".data"
        assert section_for_hunk(None, 2) == ".bss"
        assert section_for_hunk(None, 99) == ".text"  # unknown defaults to .text

    @mock.patch("subprocess.run")
    def test_multi_hunk_selects_data_section(self, mock_run):
        """A hit in hunk 1 with .map should use --section=.data."""
        # count_sections returns 3 (multi-hunk)
        mock_run.side_effect = [
            mock.Mock(
                stdout=(
                    "  0 .text  00001000  00000000  00000000  00000034  2**2\n"
                    "  1 .data  00000100  00001000  00001000  00001034  2**2\n"
                    "  2 .bss   00000050  00001100  00001100  00001134  2**2\n"
                ),
                returncode=0,
            ),
            # addr2line for the data hunk
            mock.Mock(
                stdout="global_table\nported/data.c:10\n",
                returncode=0,
            ),
        ]

        hits_data = {
            "hits": [{
                "type": "LONG-WRITE",
                "address": "0x00001234",
                "pc": "0x00000100",
                "data": "0x00000000",
                "segtracker": {
                    "name": "testprog",
                    "hunk": 1,
                    "offset": "0x00000010",
                },
            }],
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            hits_path = os.path.join(tmpdir, "hits.json")
            binary_path = os.path.join(tmpdir, "testprog")
            map_path = os.path.join(tmpdir, "testprog.map")

            with open(hits_path, "w") as f:
                json.dump(hits_data, f)
            with open(binary_path, "w") as f:
                f.write("fake binary")
            with open(map_path, "w") as f:
                f.write(".text   0x00000000   0x1000\n")
                f.write(".data   0x00001000   0x0100\n")
                f.write(".bss    0x00001100   0x0050\n")

            args = mock.Mock()
            args.hits_json = hits_path
            args.binary = binary_path

            with mock.patch("sys.stdout"):
                cmd_map(args)

            # Verify addr2line was called with --section=.data (hunk 1)
            addr2line_call = mock_run.call_args_list[1][0][0]
            assert "--section=.data" in addr2line_call


# --------------------------------------------------------------------------- #
# T8: addr2line map — no debug info
# --------------------------------------------------------------------------- #

class TestT8Addr2LineNoDebugInfo:
    """T8: addr2line returns '??:0', verify graceful error message."""

    @mock.patch("subprocess.run")
    def test_no_debug_info_returns_none(self, mock_run):
        mock_run.return_value = mock.Mock(
            stdout="??\n??:0\n",
            returncode=0,
        )

        source_file, line_num, func = run_addr2line(
            "/fake/binary", ".text", "0x00000080"
        )

        assert source_file is None
        assert line_num is None
        assert func is None

    @mock.patch("subprocess.run")
    def test_no_debug_info_produces_error_in_map(self, mock_run):
        """cmd_map should produce 'no debug info' error for ??:0."""
        mock_run.side_effect = [
            # objdump -h: single section
            mock.Mock(
                stdout="  0 .text  00001000  00000000  00000000  00000034  2**2\n",
                returncode=0,
            ),
            # addr2line: no debug info
            mock.Mock(
                stdout="??\n??:0\n",
                returncode=0,
            ),
        ]

        hits_data = {
            "hits": [{
                "type": "LONG-READ",
                "pc": "0x07895CA8",
                "segtracker": {
                    "name": "testprog",
                    "hunk": 0,
                    "offset": "0x00000080",
                },
            }],
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            hits_path = os.path.join(tmpdir, "hits.json")
            binary_path = os.path.join(tmpdir, "testprog")
            with open(hits_path, "w") as f:
                json.dump(hits_data, f)
            with open(binary_path, "w") as f:
                f.write("fake binary")

            args = mock.Mock()
            args.hits_json = hits_path
            args.binary = binary_path

            captured = []
            original_dump = json.dump

            def capture_dump(obj, fp, **kwargs):
                captured.append(obj)
                return original_dump(obj, fp, **kwargs)

            with mock.patch("sys.stdout"):
                with mock.patch.object(json, "dump", side_effect=capture_dump):
                    cmd_map(args)

            assert len(captured) == 1
            mapped = captured[0]["mapped_hits"]
            assert len(mapped) == 1
            assert "no debug info" in mapped[0]["error"]
            assert "gstabs" in mapped[0]["error"]


# --------------------------------------------------------------------------- #
# T9: addr2line map — multi-hunk without .map
# --------------------------------------------------------------------------- #

class TestT9MultiHunkNoMap:
    """T9: Verify error when multi-hunk binary has no .map file."""

    @mock.patch("subprocess.run")
    def test_multi_hunk_without_map_exits_with_error(self, mock_run):
        # objdump -h: 3 sections (multi-hunk)
        mock_run.return_value = mock.Mock(
            stdout=(
                "  0 .text  00001000  00000000  00000000  00000034  2**2\n"
                "  1 .data  00000100  00001000  00001000  00001034  2**2\n"
                "  2 .bss   00000050  00001100  00001100  00001134  2**2\n"
            ),
            returncode=0,
        )

        hits_data = {
            "hits": [{
                "type": "LONG-READ",
                "pc": "0x07895CA8",
                "segtracker": {
                    "name": "testprog",
                    "hunk": 0,
                    "offset": "0x00000080",
                },
            }],
        }

        with tempfile.TemporaryDirectory() as tmpdir:
            hits_path = os.path.join(tmpdir, "hits.json")
            binary_path = os.path.join(tmpdir, "testprog")
            with open(hits_path, "w") as f:
                json.dump(hits_data, f)
            with open(binary_path, "w") as f:
                f.write("fake binary")
            # Deliberately do NOT create testprog.map

            args = mock.Mock()
            args.hits_json = hits_path
            args.binary = binary_path

            with pytest.raises(SystemExit) as exc_info:
                with mock.patch("sys.stderr"):
                    cmd_map(args)

            assert exc_info.value.code == 1


# --------------------------------------------------------------------------- #
# T10: Crash classification — NULL deref
# --------------------------------------------------------------------------- #

class TestT10CrashClassificationNull:
    """T10: Address < 0x1000 should produce crash_type containing 'NULL pointer'."""

    def test_address_zero_is_null_pointer(self):
        result = classify_crash(0x00000000, "WORD-WRITE")
        assert "NULL pointer" in result

    def test_small_offset_is_null_pointer(self):
        result = classify_crash(0x00000014, "LONG-READ")
        assert "NULL pointer" in result
        assert "0x14" in result.upper() or "0x14" in result

    def test_address_at_boundary(self):
        result = classify_crash(0x00000FFF, "BYTE-READ")
        assert "NULL pointer" in result

    def test_address_above_boundary_not_null(self):
        result = classify_crash(0x00001000, "LONG-READ")
        assert "NULL pointer" not in result

    def test_null_pointer_includes_hit_type(self):
        result = classify_crash(0x00000000, "LONG-WRITE")
        assert "long write" in result


# --------------------------------------------------------------------------- #
# T11: Crash classification — Mungwall
# --------------------------------------------------------------------------- #

class TestT11CrashClassificationMungwall:
    """T11: Address 0xDEADBEEF should produce crash_type containing 'use-after-free'."""

    def test_deadbeef_classified_as_use_after_free(self):
        result = classify_crash(0xDEADBEEF, "LONG-READ")
        assert "use-after-free" in result
        assert "Mungwall" in result

    def test_abadcafe_classified_as_pre_fill(self):
        result = classify_crash(0xABADCAFE, "WORD-WRITE")
        assert "pre-fill" in result
        assert "Mungwall" in result

    def test_cccccccc_classified_as_uninitialized(self):
        result = classify_crash(0xCCCCCCCC, "BYTE-READ")
        assert "uninitialized" in result
        assert "Mungwall" in result

    def test_mungwall_takes_priority_over_type(self):
        """Mungwall classification should replace the raw hit type."""
        result = classify_crash(0xDEADBEEF, "LONG-WRITE")
        # Should NOT just be "LONG-WRITE"
        assert result != "LONG-WRITE"
        assert "Mungwall" in result

    def test_normal_address_returns_hit_type(self):
        result = classify_crash(0x07895CA4, "LONG-READ")
        assert result == "LONG-READ"


# --------------------------------------------------------------------------- #
# T12: Empty/malformed input
# --------------------------------------------------------------------------- #

class TestT12EmptyAndMalformedInput:
    """T12: Graceful degradation for empty or garbled input."""

    def test_empty_file_zero_hits(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, "")
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 0
        assert result["hits"] == []
        assert result["summary"]["crash_types"] == []

    def test_garbled_text_zero_hits(self):
        garbled = """\
This is not Enforcer output at all.
It's just random text that shouldn't crash the parser.
Some numbers: 12345 ABCDEF
Special chars: !@#$%^&*()
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, garbled)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 0
        assert result["hits"] == []

    def test_blank_lines_only(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, "\n\n\n\n\n")
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 0

    def test_partial_hit_missing_fields(self):
        """A hit line followed by unrelated text should still produce a hit."""
        partial = """\
LONG-READ from 00000000                        PC: 07895CA4
This is not a valid status line.
Neither is this.
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, partial)
            result = parse_enforcer_log(path)

        # The hit line itself should be parsed even without subsequent fields
        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]
        assert hit["type"] == "LONG-READ"
        assert hit["address"] == "0x00000000"
        assert hit["pc"] == "0x07895CA4"

    def test_interleaved_garbage(self):
        """Garbage lines between valid hits should not corrupt parsing."""
        interleaved = """\
WORD-WRITE to  00000000        data=4444       PC: 07895CA4
USP:  078D692C SR: 0000 SW: 0729  (U0)(-)(-)  TCB: 078A2690
Data: DDDD0000 DDDD1111 DDDD2222 DDDD3333 DDDD4444 DDDD5555 DDDD6666 DDDD7777
Addr: AAAA0000 AAAA1111 AAAA2222 AAAA3333 AAAA4444 AAAA5555 07800804 --------
This garbage line should be ignored.
Name: "New_Shell"  CLI: "lawbreaker"  Hunk 0000 Offset 0000007C
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = _write_fixture(tmpdir, interleaved)
            result = parse_enforcer_log(path)

        assert result["summary"]["total_hits"] == 1
        hit = result["hits"][0]
        assert hit["task_name"] == "New_Shell"
        assert hit["cli_name"] == "lawbreaker"


# --------------------------------------------------------------------------- #
# parse_flags utility
# --------------------------------------------------------------------------- #

class TestParseFlags:
    """Unit tests for the flag parsing helper."""

    def test_user_mode(self):
        assert parse_flags("U0")["user_mode"] is True
        assert parse_flags("U0")["forbid"] is False
        assert parse_flags("U0")["disable"] is False

    def test_forbid(self):
        assert parse_flags("F")["forbid"] is True

    def test_disable(self):
        assert parse_flags("D")["disable"] is True

    def test_inactive(self):
        result = parse_flags("-")
        assert result["user_mode"] is False
        assert result["forbid"] is False
        assert result["disable"] is False

    def test_combined_flags(self):
        # From the LONG-READ fixture: (U0)(F)(-)
        # The flag string as parsed from between parens is the full string
        result = parse_flags("U0")
        assert result["user_mode"] is True
