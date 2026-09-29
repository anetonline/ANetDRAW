#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Generate the reviewable public acceptance contract inventory."""

from __future__ import annotations

import json
import sys
from pathlib import Path

import check_public_contract


ROOT = Path(__file__).resolve().parent.parent
ALL = ["windows", "unix", "dos16", "dos32"]
DOS = ["dos16", "dos32"]
PLATFORM_CONSTANTS = {
    "ODPLAT_WIN32": ["windows"],
    "ODPLAT_NIX": ["unix"],
    "ODPLAT_DOS": ["dos16"],
    "ODPLAT_DOS32": ["dos32"],
    "OD_WIN32_STATIC": ["windows"],
    "OD_DLL": ["windows"],
}

TESTS = {
    "surface.compile": {"tier": "push", "description": "C and C++ public-header compilation"},
    "surface.abi": {"tier": "push", "description": "Public type sizes and tODControl layout"},
    "surface.constants": {"tier": "push", "description": "Documented constant values"},
    "surface.aliases": {"tier": "push", "description": "Compatibility alias identity"},
    "symbols.exports": {"tier": "push", "description": "Every declared API symbol is linkable/exported"},
    "control.storage": {"tier": "push", "description": "Global identity, initial storage, and field addressability"},
    "lifecycle.defaults": {"tier": "push", "description": "Initialization defaults and caller override behavior"},
    "command-line.behavior": {"tier": "push", "description": "Command-line parsing and callbacks"},
    "dropfiles.all": {"tier": "push", "description": "Every built-in and custom drop-file contract"},
    "terminal.screen": {"tier": "push", "description": "Public screen, display, cursor, and window operations"},
    "terminal.socket": {"tier": "extended", "description": "Transport-neutral remote output, input, and autodetection"},
    "input.interactive": {"tier": "extended", "description": "Scripted input, editor, menu, page, and chat behavior"},
    "process.spawn": {"tier": "push", "description": "Spawn, environment, logging, sleep, and kernel behavior"},
    "components.lifecycle": {"tier": "push", "description": "Optional component setup and callbacks"},
    "personalities.builtin": {"tier": "push", "description": "Built-in personality lifecycle"},
    "personality.sdk": {"tier": "push", "description": "DOS personality SDK compile and screen-cell behavior"},
    "windows.control": {"tier": "push", "description": "Windows-only public control fields"},
    "reservation.named": {"tier": "push", "description": "Named filesystem reservation lifecycle"},
}


def function_evidence(name: str) -> list[str]:
    groups = {
        "ODConfigInit": "components.lifecycle",
        "ODLogEnable": "components.lifecycle",
        "ODMPSEnable": "components.lifecycle",
        "od_add_personality": "personalities.builtin",
        "od_set_personality": "personalities.builtin",
        "od_autodetect": "terminal.socket",
        "od_carrier": "terminal.socket",
        "od_chat": "input.interactive",
        "od_clear_keybuffer": "input.interactive",
        "od_get_answer": "input.interactive",
        "od_get_input": "input.interactive",
        "od_get_input_until": "input.interactive",
        "od_get_key": "input.interactive",
        "od_get_time": "input.interactive",
        "od_get_user_8bit": "dropfiles.all",
        "od_input_str": "input.interactive",
        "od_key_pending": "input.interactive",
        "od_edit_str": "input.interactive",
        "od_multiline_edit": "input.interactive",
        "od_page": "input.interactive",
        "od_popup_menu": "input.interactive",
        "od_hotkey_menu": "input.interactive",
        "od_spawn": "process.spawn",
        "od_spawnvpe": "process.spawn",
        "od_log_open": "process.spawn",
        "od_log_write": "process.spawn",
        "od_sleep": "process.spawn",
        "od_kernel": "process.spawn",
        "od_parse_cmd_line": "command-line.behavior",
        "od_parse_cmd_line_cons": "command-line.behavior",
        "od_split_cmd_line": "command-line.behavior",
        "od_free_split_cmd_line": "command-line.behavior",
        "od_set_user_8bit": "dropfiles.all",
        "od_reserve_configure": "reservation.named",
        "od_reserve_request": "reservation.named",
        "od_reserve_wait": "reservation.named",
        "od_reserve_wait_until": "reservation.named",
        "od_reserve_end": "reservation.named",
    }
    if name.startswith("pdef_"):
        group = "personalities.builtin"
    elif name in groups:
        group = groups[name]
    elif name in {"od_init", "od_exit", "od_control_get", "od_set_port",
                  "od_set_dtr", "od_set_statusline", "od_color_config"}:
        group = "lifecycle.defaults"
    else:
        group = "terminal.screen"
    return ["symbols.exports", group]


def field_evidence(name: str) -> list[str]:
    result = ["surface.abi", "control.storage"]
    if name in {"od_app_icon", "od_cmd_show"}:
        result.append("windows.control")
    elif name.startswith("key_") or name.startswith("od_hot_") or name == "od_num_keys":
        result.append("input.interactive")
    elif name.startswith("user_") or name.startswith("system_") or name.startswith("event_") \
            or name.startswith("timelog_") or name in {"bIsCoSysop", "bIsSysop"}:
        result.append("dropfiles.all")
    elif "callback" in name or "function" in name or name.startswith("od_cbefore") \
            or name.startswith("od_cafter") or name in {"od_no_file_func", "od_before_exit",
            "od_ker_exec", "od_local_input", "od_time_msg_func", "od_cmd_line_flag_handler",
            "od_cmd_line_handler", "od_cmd_line_help_func"}:
        result.append("components.lifecycle")
    else:
        result.append("lifecycle.defaults")
    return result


def entry(kind: str, name: str) -> dict[str, object]:
    platforms = DOS if kind.startswith("personality-") else ALL
    if kind == "control-field" and name in {"od_app_icon", "od_cmd_show"}:
        platforms = ["windows"]
    elif kind == "function" and name == "od_parse_cmd_line_cons":
        platforms = ["windows"]
    elif kind == "constant" and name in PLATFORM_CONSTANTS:
        platforms = PLATFORM_CONSTANTS[name]
    if kind == "function":
        evidence = function_evidence(name)
    elif kind == "personality-function":
        evidence = ["personality.sdk"]
    elif kind == "control-field":
        evidence = field_evidence(name)
    elif kind == "constant":
        evidence = ["surface.constants"]
    elif kind == "compatibility-alias":
        evidence = ["surface.aliases"]
    elif kind == "type":
        evidence = ["surface.compile", "surface.abi"]
    elif kind == "global":
        evidence = ["symbols.exports", "control.storage"]
    else:
        evidence = ["personality.sdk"]
    return {"kind": kind, "name": name, "platforms": platforms,
            "evidence": evidence}


def main() -> int:
    data = {
        "version": 1,
        "tests": TESTS,
        "coverage": [entry(kind, name) for kind, name in
                     sorted(check_public_contract.inventory())],
    }
    json.dump(data, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
