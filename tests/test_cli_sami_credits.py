#!/usr/bin/env python3
"""Run output regressions against a built CCExtractor: python3 tests/test_cli_sami_credits.py BINARY."""

import re
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


BINARY = Path(sys.argv.pop(1)).resolve()
SAMPLE = Path(__file__).resolve().parent / "samples" / "cli_output.scc"
ENCODINGS = (("--utf8", "utf-8-sig"), ("--latin1", "latin-1"), ("--unicode", "utf-16"))


class CliOutputTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name) / "output.smi"

    def run_ccx(self, *options):
        command = [str(BINARY), str(SAMPLE), *options, "-o", str(self.output)]
        result = subprocess.run(command, capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        return result

    def test_start_credits_have_a_clear_cue(self):
        label = "Example start credits"
        for flag, encoding in ENCODINGS:
            with self.subTest(encoding=encoding):
                self.run_ccx(
                    "--sami", flag,
                    "--startcreditstext", label,
                    "--startcreditsforatleast", "1",
                    "--startcreditsforatmost", "2",
                )
                text = self.output.read_bytes().decode(encoding)
                self.assertEqual(text.count(label), 1)
                cue = re.search(
                    r'<SYNC start=(\d+)><P class="UNKNOWNCC">\s*'
                    + re.escape(label)
                    + r'<br>\s*</P></SYNC>\s*'
                    + r'<SYNC start=(\d+)><P class="UNKNOWNCC">&nbsp;</P></SYNC>',
                    text,
                )
                self.assertIsNotNone(cue, text)
                start, end = map(int, cue.groups())
                self.assertEqual(end - start, 2000)
                self.assertEqual(text.count("<SYNC start="), text.count("</SYNC>"))
                self.assertIn("HELLO WORLD", text)
                self.assertIn("BYE WORLD", text)


if __name__ == "__main__":
    unittest.main(verbosity=2)
