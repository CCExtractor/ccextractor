#!/usr/bin/env python3
"""Run output regressions against a built CCExtractor: python3 tests/test_cli_sami_alias.py BINARY."""

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

    def test_sami_aliases_match_explicit_format(self):
        for flag, encoding in ENCODINGS:
            self.run_ccx("--out", "sami", flag)
            expected = self.output.read_bytes()
            for alias in ("--sami", "--smi", "-smi"):
                with self.subTest(alias=alias, encoding=encoding):
                    self.run_ccx(alias, flag)
                    actual = self.output.read_bytes()
                    self.assertEqual(actual, expected)
                    text = actual.decode(encoding)
                    self.assertTrue(text.startswith("<SAMI>"))
                    self.assertIn("HELLO WORLD", text)
                    self.assertIn("BYE WORLD", text)


if __name__ == "__main__":
    unittest.main(verbosity=2)
