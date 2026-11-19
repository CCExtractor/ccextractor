#!/usr/bin/env python3
"""Run output regressions against a built CCExtractor: python3 tests/test_cli_webvtt_stdout.py BINARY."""

from pathlib import Path
import subprocess
import sys
import unittest


BINARY = Path(sys.argv.pop(1)).resolve()
SAMPLE = Path(__file__).resolve().parent / "samples" / "cli_output.scc"
ENCODINGS = (("--utf8", "utf-8-sig"), ("--latin1", "latin-1"), ("--unicode", "utf-16"))


class CliOutputTests(unittest.TestCase):
    def run_ccx(self, *options):
        command = [str(BINARY), str(SAMPLE), *options]
        result = subprocess.run(command, capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        return result

    def test_webvtt_stdout_contains_only_subtitles(self):
        for flag, _ in ENCODINGS:
            for format_options in (("--webvtt",), ("--out", "webvtt")):
                for stdout in ("--stdout", "-stdout"):
                    for quiet in (False, True):
                        with self.subTest(
                            flag=flag, format=format_options, stdout=stdout, quiet=quiet
                        ):
                            options = [*format_options, flag, stdout, "--no-bom"]
                            if quiet:
                                options.append("--quiet")
                            result = self.run_ccx(*options)
                            text = result.stdout.decode("utf-8")
                            self.assertTrue(text.startswith("WEBVTT\r\n"), text[:120])
                            self.assertNotIn("forcing UTF-8", text)
                            self.assertIn("HELLO WORLD", text)
                            self.assertIn("BYE WORLD", text)
                            self.assertEqual(text.count(" --> "), 2)
                            notice = b"forcing UTF-8" in result.stderr
                            self.assertEqual(notice, flag != "--utf8" and not quiet)


if __name__ == "__main__":
    unittest.main(verbosity=2)
