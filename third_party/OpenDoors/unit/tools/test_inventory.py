#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import ROOT, build_inventory, scan_asm, scan_c  # noqa: E402


class InventoryTests(unittest.TestCase):
    def test_function_platform_override(self):
        inventory = build_inventory()
        source = next(item for item in inventory["sources"]
                      if item["path"] == "src/ODStat.c")
        function = next(item for item in source["functions"]
                        if item["name"] == "ODStatForceStatusUpdate")
        self.assertEqual(function["platforms"],
                         ["windows", "dos16", "dos32"])

    def test_guarded_platform_helpers_have_explicit_platforms(self):
        inventory = build_inventory()
        expected = {
            ("src/ODPlat.c", "ODPlatYield"): ["dos16"],
            ("src/ODSpawn.c", "ODUnixExecProgram"): ["unix"],
        }
        actual = {}
        for source in inventory["sources"]:
            for function in source["functions"]:
                key = (source["path"], function["name"])
                if key in expected:
                    actual[key] = function["platforms"]
        self.assertEqual(actual, expected)

    def test_win32_screen_helpers_are_windows_only(self):
        inventory = build_inventory()
        source = next(item for item in inventory["sources"]
                      if item["path"] == "src/ODScrn.c")
        expected = {
            "ODScrnCreateWin", "ODScrnWindowProc", "ODScrnPaint",
            "ODScrnInvalidate", "ODScrnPublish", "ODScrnSetCurrentFont",
            "ODScrnAdjustWindows", "ODScrnStartWindow", "ODScrnStopWindow",
            "ODScrnSetFocusToWindow", "ODScrnSetWinCaretPos",
        }
        actual = {item["name"]: item["platforms"]
                  for item in source["functions"] if item["name"] in expected}
        self.assertEqual(set(actual), expected)
        self.assertTrue(all(platforms == ["windows"]
                            for platforms in actual.values()))

    def test_text_window_helpers_include_console_windows(self):
        inventory = build_inventory()
        source = next(item for item in inventory["sources"]
                      if item["path"] == "src/ODScrn.c")
        expected = {"ODScrnCreateWindow", "ODScrnDestroyWindow"}
        actual = {item["name"]: item["platforms"]
                  for item in source["functions"] if item["name"] in expected}
        self.assertEqual(set(actual), expected)
        self.assertTrue(all(platforms ==
                            ["unix", "windows", "dos16", "dos32"]
                            for platforms in actual.values()))

    def test_text_mode_local_input_includes_console_windows(self):
        inventory = build_inventory()
        source = next(item for item in inventory["sources"]
                      if item["path"] == "src/ODScrn.c")
        function = next(item for item in source["functions"]
                        if item["name"] == "ODScrnLocalInput")
        self.assertEqual(function["platforms"],
                         ["windows", "dos16", "dos32"])

    def test_kernel_ui_queue_helper_is_windows_only(self):
        inventory = build_inventory()
        source = next(item for item in inventory["sources"]
                      if item["path"] == "src/ODKrnl.c")
        platforms = {item["name"]: item["platforms"]
                     for item in source["functions"]}
        self.assertEqual(platforms["ODKrnlRequestShutdown"], ["windows"])
        signals = {"sig_run_kernel", "sig_get_char", "sig_no_carrier"}
        self.assertTrue(signals.isdisjoint(platforms))

    def test_c_scanner_ignores_comments_literals_and_control_blocks(self):
        source = r'''
/* void omitted(void) {} */
static int first(int value)
{
    const char *text = "not_a_function() {";
    if (value) { return 1; }
    return 0;
}

#if NEVER
void conditional(void) { }
#endif
'''
        functions = scan_c(source)
        self.assertEqual([item.name for item in functions],
                         ["first", "conditional"])
        self.assertEqual(functions[0].body_line, 4)
        self.assertEqual(functions[0].end_line, 8)

    def test_assembler_scanner_coalesces_memory_model_declarations(self):
        source = """\
#if LCODE
sample PROC far
#else
sample PROC near
#endif
  ret
sample ENDP
"""
        functions = scan_asm(source)
        self.assertEqual(len(functions), 1)
        self.assertEqual(functions[0].name, "sample")
        self.assertEqual(functions[0].end_line, 7)

    def test_preprocessor_between_signature_and_body_is_ignored(self):
        source = """\
#ifdef WINDOWS
void selected(int value)
#else
void selected(int value, char **items)
#endif
{
   (void)value;
}
"""
        functions = scan_c(source)
        self.assertEqual([item.name for item in functions], ["selected"])
        self.assertEqual(functions[0].body_line, 6)

    def test_discovers_a_function_returning_a_function_pointer(self):
        source = """\
static void (*vector_get(unsigned vector))(void)
{
   return 0;
}
"""
        functions = scan_c(source)
        self.assertEqual([item.name for item in functions], ["vector_get"])

    def test_literal_if_zero_does_not_unbalance_alternative_syntax(self):
        source = """\
void first(int value)
{
   if(value
#if 0
      && another) {
#else
      ) {
#endif
      value = 0;
   }
}
void second(void) { }
"""
        self.assertEqual([item.name for item in scan_c(source)],
                         ["first", "second"])

    def test_discovers_every_odgetin_definition(self):
        source = (ROOT / "src" / "ODGetIn.c").read_text(encoding="latin-1")
        self.assertEqual([item.name for item in scan_c(source)], [
            "ODGetInputWait", "ODGetInputDeadlineSlice",
            "ODGetInputWaitUntil", "ODGetInputCore", "od_get_input",
            "od_get_input_until", "ODLongestFullCode",
            "ODHaveStartOfSequence", "ODGetCodeIfLongest", "ODShiftSeq",
        ])


if __name__ == "__main__":
    unittest.main()
