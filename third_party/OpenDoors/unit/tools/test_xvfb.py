import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class UnitXvfbTests(unittest.TestCase):
    def test_explicit_watcom_root_is_exported_to_runner(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            watcom = directory / "watcom"
            binl = watcom / "binl"
            binl.mkdir(parents=True)
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            python = directory / "python3"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            python.write_text(
                "#!/bin/sh\n"
                "test \"$WATCOM\" = \"$UNIT_XVFB_TEST_WATCOM\"\n"
                "test \"${PATH%%:*}\" = \"$WATCOM/binl\"\n",
                encoding="ascii")
            for executable in (xvfb, xdpyinfo, python):
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment.pop("WATCOM", None)
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)
            environment["UNIT_XVFB_TEST_WATCOM"] = str(watcom)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":199",
                 "--watcom", str(watcom), "run", "--help"], cwd=ROOT,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_starts_x_server_before_dispatching_runner(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            python = directory / "python3"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            python.write_text(
                "#!/bin/sh\n"
                "test \"$DISPLAY\" = :197\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            xvfb.chmod(xvfb.stat().st_mode | stat.S_IXUSR)
            xdpyinfo.chmod(xdpyinfo.stat().st_mode | stat.S_IXUSR)
            python.chmod(python.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":197",
                 "run", "--help"], cwd=ROOT, env=environment,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_debug_mode_runs_dosbox_on_private_display(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            dosbox = directory / "dosbox"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            dosbox.write_text(
                "#!/bin/sh\n"
                "test \"$DISPLAY\" = :198\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n"
                "test \"$1\" = -conf\n"
                "test \"$2\" = debug.conf\n",
                encoding="ascii")
            for executable in (xvfb, xdpyinfo, dosbox):
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":198",
                 "debug", str(dosbox), "-conf", "debug.conf"], cwd=ROOT,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_serial_mode_runs_the_fixed_controller_on_private_display(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            python = directory / "python3"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            python.write_text(
                "#!/bin/sh\n"
                "test \"$DISPLAY\" = :196\n"
                "test \"$1\" = tests/acceptance/dos_serial_controller.py\n"
                "test \"$2\" = /test/dosbox\n"
                "test \"$3\" = /test/door.exe\n",
                encoding="ascii")
            for executable in (xvfb, xdpyinfo, python):
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":196",
                 "serial", "/test/dosbox", "/test/door.exe"], cwd=ROOT,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_dos16_build_mode_uses_watcom_and_ninja(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            configure_marker = directory / "configured"
            watcom = directory / "watcom"
            (watcom / "binl").mkdir(parents=True)
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            cmake = directory / "cmake"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            cmake.write_text(
                "#!/bin/sh\n"
                "test \"$CC\" = wcl\n"
                "if test \"$1\" = -S; then\n"
                "  test \"$2\" = dos\n"
                "  test \"$3\" = -B\n"
                "  test \"$4\" = /test/build\n"
                "  test \"$5\" = -G\n"
                "  test \"$6\" = Ninja\n"
                "  : >\"$UNIT_XVFB_CONFIGURE_MARKER\"\n"
                "else\n"
                "  test \"$1\" = --build\n"
                "  test \"$2\" = /test/build\n"
                "  test -f \"$UNIT_XVFB_CONFIGURE_MARKER\"\n"
                "fi\n",
                encoding="ascii")
            for executable in (xvfb, xdpyinfo, cmake):
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)
            environment["UNIT_XVFB_CONFIGURE_MARKER"] = str(configure_marker)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":195",
                 "--watcom", str(watcom), "dos16-build", "/test/build"],
                cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_dos32_build_mode_uses_watcom_and_ninja(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            marker = directory / "xvfb-started"
            configure_marker = directory / "configured"
            watcom = directory / "watcom"
            (watcom / "binl").mkdir(parents=True)
            xvfb = directory / "Xvfb"
            xdpyinfo = directory / "xdpyinfo"
            cmake = directory / "cmake"
            xvfb.write_text(
                "#!/bin/sh\n"
                ": >\"$UNIT_XVFB_TEST_MARKER\"\n"
                "while :; do sleep 1; done\n",
                encoding="ascii")
            xdpyinfo.write_text(
                "#!/bin/sh\n"
                "test -f \"$UNIT_XVFB_TEST_MARKER\"\n",
                encoding="ascii")
            cmake.write_text(
                "#!/bin/sh\n"
                "test \"$CC\" = wcl386\n"
                "if test \"$1\" = -S; then\n"
                "  test \"$2\" = dos\n"
                "  test \"$3\" = -B\n"
                "  test \"$4\" = /test/build32\n"
                "  test \"$5\" = -G\n"
                "  test \"$6\" = Ninja\n"
                "  test \"$8\" = CMAKE_SYSTEM_NAME=DOS\n"
                "  test \"$10\" = CMAKE_SYSTEM_PROCESSOR=I386\n"
                "  : >\"$UNIT_XVFB_CONFIGURE_MARKER\"\n"
                "else\n"
                "  test \"$1\" = --build\n"
                "  test \"$2\" = /test/build32\n"
                "  test -f \"$UNIT_XVFB_CONFIGURE_MARKER\"\n"
                "fi\n",
                encoding="ascii")
            for executable in (xvfb, xdpyinfo, cmake):
                executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            environment = os.environ.copy()
            environment["PATH"] = str(directory) + os.pathsep + os.environ["PATH"]
            environment["UNIT_XVFB_TEST_MARKER"] = str(marker)
            environment["UNIT_XVFB_CONFIGURE_MARKER"] = str(configure_marker)

            result = subprocess.run(
                [str(ROOT / "tools" / "unit-xvfb"), "--display", ":194",
                 "--watcom", str(watcom), "dos32-build", "/test/build32"],
                cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=5)

            self.assertEqual(result.returncode, 0, result.stderr.decode())


if __name__ == "__main__":
    unittest.main()
