#!/usr/bin/env python3

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from inventory import ROOT


class FrameworkAssemblyCoverageTests(unittest.TestCase):
    def test_writes_registered_assembly_branch_observations(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            source = directory / "case.c"
            executable = directory / "case"
            report = directory / "case.cov"
            source.write_text("""\
#include "ut.h"
static unsigned char observations[4] = { 1, 1, 0, 1 };
static void exercise(void) { }
static const UTTestCase cases[] = { { "asm", exercise } };
int main(int argc, char **argv)
{
   ut_cov_asm_register(observations, 2);
   return ut_run(cases, 1, argc > 1 ? argv[1] : (char *)0);
}
""", encoding="ascii")
            subprocess.run([
                "clang", "-std=c89", "-Wall", "-Wextra", "-Werror",
                "-I", str(ROOT / "unit" / "framework"),
                str(ROOT / "unit" / "framework" / "ut.c"), str(source),
                "-o", str(executable),
            ], check=True, cwd=ROOT)
            subprocess.run([str(executable), str(report)], check=True,
                           cwd=ROOT)
            self.assertEqual(report.read_text(encoding="ascii").splitlines(),
                             ["OPENDOORS-UNIT-COVERAGE 1",
                              "A 1 0", "A 1 1", "A 2 1"])


if __name__ == "__main__":
    unittest.main()
