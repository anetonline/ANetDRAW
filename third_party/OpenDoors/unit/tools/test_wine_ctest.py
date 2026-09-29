import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_executable(path: Path, contents: str) -> None:
    path.write_text(contents, encoding="ascii")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


class WineCTestTests(unittest.TestCase):
    def test_captures_crash_dump_and_restores_debugger(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            binaries = directory / "bin"
            prefix = directory / "prefix"
            wine_temp = prefix / "drive_c" / "users" / "tester" / "Temp"
            artifacts = directory / "artifacts"
            binaries.mkdir()
            wine_temp.mkdir(parents=True)

            marker = directory / "xvfb-started"
            wine_log = directory / "wine.log"
            winepath_log = directory / "winepath.log"
            make_executable(
                binaries / "Xvfb",
                "#!/bin/sh\n"
                ": >\"$WINE_CTEST_XVFB_MARKER\"\n"
                "while :; do sleep 1; done\n")
            make_executable(
                binaries / "xdpyinfo",
                "#!/bin/sh\n"
                "test -f \"$WINE_CTEST_XVFB_MARKER\"\n")
            make_executable(
                binaries / "wine",
                "#!/bin/sh\n"
                "printf '%s\\n' \"$*\" >>\"$WINE_CTEST_WINE_LOG\"\n"
                "case \"$1 $2\" in\n"
                "  'reg export')\n"
                "    backup=$4\n"
                "    backup=${backup#Z:}\n"
                "    backup=$(printf '%s' \"$backup\" | tr '\\\\' '/')\n"
                "    : >\"$backup\"\n"
                "    ;;\n"
                "  'winedbg.exe --command') echo 'mock backtrace' >&2 ;;\n"
                "esac\n")
            make_executable(
                binaries / "ctest",
                "#!/bin/sh\n"
                "test \"$DISPLAY\" = :197\n"
                "printf '%s' \"$WINEPATH\" >\"$WINE_CTEST_WINEPATH_LOG\"\n"
                "printf 'minidump' >\"$WINE_CTEST_FAKE_TEMP/WD1234.tmp\"\n"
                "exit 7\n")

            environment = os.environ.copy()
            environment["PATH"] = str(binaries) + os.pathsep + environment["PATH"]
            environment["WINE_CTEST_XVFB_MARKER"] = str(marker)
            environment["WINE_CTEST_WINE_LOG"] = str(wine_log)
            environment["WINE_CTEST_WINEPATH_LOG"] = str(winepath_log)
            environment["WINE_CTEST_FAKE_TEMP"] = str(wine_temp)

            result = subprocess.run(
                [str(ROOT / "tools" / "wine-ctest"),
                 "--display", ":197", "--wine", str(binaries / "wine"),
                 "--prefix", str(prefix), "--artifacts", str(artifacts),
                 "--dll-directory", str(binaries),
                 "--test-dir", "build/wine"],
                cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            output = result.stdout.decode() + result.stderr.decode()
            self.assertEqual(result.returncode, 7, output)
            dumps = list(artifacts.glob("*.mdmp"))
            diagnostics = list(artifacts.glob("*.txt"))
            self.assertEqual(len(dumps), 1, output)
            self.assertEqual(len(diagnostics), 1, output)
            self.assertIn("mock backtrace", diagnostics[0].read_text())
            self.assertIn(str(dumps[0]), output)
            wine_calls = wine_log.read_text()
            self.assertIn("reg add", wine_calls)
            self.assertIn("winedbg --minidump %ld %ld", wine_calls)
            self.assertIn("reg import", wine_calls)
            self.assertEqual(winepath_log.read_text(),
                             "Z:" + str(binaries).replace("/", "\\"))


if __name__ == "__main__":
    unittest.main()
