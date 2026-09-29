#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Transport-neutral protocol for public OpenDoors acceptance doors."""

from __future__ import annotations

import select
import socket
import time
from pathlib import Path


SCENARIOS = (
    "input",
    "interactive",
    "edit",
    "display",
    "emulation",
    "listing",
    "popup",
    "session",
)

LONG_DESCRIPTION = b"DESC-" + b"1234567890" * 6
FIXTURE_DIRECTORIES = ("ODAREA",)

FIXTURES = {
    "ODFILE.ASC": b"FILE-CONTENT\r\n",
    "ODFILE.ans": b"ANSI-CONTENT\r\n",
    "ODSECT.ASC": (
        b"@#FIRST\r\nWRONG-SECTION\r\n"
        b"@#SECOND\r\nSECTION-CONTENT\r\n"
    ),
    "ODMENU.ASC": b"MENU-CONTENT\r\n",
    "ODLONG.ASC": b"LONG-MENU-" + b"0123456789" * 80 + b"\r\n",
    "ODFMT.rip": b"FORMAT-RIP\r\n",
    "ODFMT.avt": b"FORMAT-AVATAR\r\n",
    "ODFMT.ans": b"FORMAT-ANSI\r\n",
    "ODFMT.asc": b"FORMAT-ASCII\r\n",
    "ODPAGE.asc": (
        b"PAGE-LINE-1\r\nPAGE-LINE-2\r\n"
        b"PAGE-LINE-3\r\nPAGE-LINE-4\r\n"
    ),
    "ODAREA/FILES.BBS": (
        b" LIST-TITLE\r\n\r\n"
        b"EXIST.TXT Existing entry\r\n"
        b"MATCH?.DAT Wildcard entry\r\n"
        b"MISSING.ZIP Offline entry\r\n"
        b"LONG.TXT " + LONG_DESCRIPTION + b"\r\n"
        b"SKIP.TXT " + b"Z" * 600 + b"\r\n"
        b"AFTER.TXT AFTER-LONG-LINE\r\n"
    ),
    "ODAREA/EXIST.TXT": b"existing\n",
    "ODAREA/MATCH1.DAT": b"match one\n",
    "ODAREA/MATCH2.DAT": b"match two\n",
    "ODAREA/LONG.TXT": b"long\n",
    "ODAREA/AFTER.TXT": b"after\n",
    "ODCOLOR.BBS": b"COLOR.TXT Color entry\r\n",
    "COLOR.TXT": b"color\n",
    "ODPAGE.BBS": (
        b" PAGE-LINE-1\r\n PAGE-LINE-2\r\n"
        b" PAGE-LINE-3\r\n PAGE-LINE-4\r\n"
    ),
    "ODCANCEL.BBS": (
        b" CANCEL-LINE-1\r\n CANCEL-LINE-2\r\n"
        b" CANCEL-LINE-3\r\n CANCEL-LINE-4\r\n"
    ),
    "ODBADTOK.BBS": b"A" * 80 + b" invalid token\r\n",
    "FILES.BBS": b"ITEM.TXT Acceptance listed file\r\n",
    "ITEM.TXT": b"fixture\n",
}


def receive_until(
    peer: socket.socket, marker: bytes, transcript: bytearray, timeout: int = 10
) -> None:
    deadline = time.monotonic() + timeout
    while marker not in transcript:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise RuntimeError(
                f"timed out waiting for {marker!r}; got {bytes(transcript)!r}"
            )
        readable, _, _ = select.select([peer], [], [], remaining)
        if not readable:
            continue
        chunk = peer.recv(4096)
        if not chunk:
            raise RuntimeError(
                f"connection closed waiting for {marker!r}; "
                f"got {bytes(transcript)!r}"
            )
        transcript.extend(chunk)


def receive_until_any(
    peer: socket.socket,
    markers: tuple[bytes, ...],
    transcript: bytearray,
    timeout: int = 10,
) -> bytes:
    deadline = time.monotonic() + timeout
    while True:
        for marker in markers:
            if marker in transcript:
                return marker
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise RuntimeError(
                f"timed out waiting for one of {markers!r}; "
                f"got {bytes(transcript)!r}"
            )
        readable, _, _ = select.select([peer], [], [], remaining)
        if not readable:
            continue
        chunk = peer.recv(4096)
        if not chunk:
            raise RuntimeError(
                f"connection closed waiting for one of {markers!r}; "
                f"got {bytes(transcript)!r}"
            )
        transcript.extend(chunk)


def create_fixtures(directory: Path) -> None:
    for name in FIXTURE_DIRECTORIES:
        (directory / name).mkdir(exist_ok=True)
    for name, contents in FIXTURES.items():
        (directory / name).write_bytes(contents)


def remove_fixtures(directory: Path) -> None:
    for name in FIXTURES:
        (directory / name).unlink(missing_ok=True)
    for name in reversed(FIXTURE_DIRECTORIES):
        path = directory / name
        if path.exists():
            path.rmdir()


def finish(peer: socket.socket, transcript: bytearray, marker: bytes,
           timeout: int) -> None:
    receive_until(peer, marker, transcript, timeout)
    peer.sendall(b"X")


def drive_input(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"INPUT-CHAR", transcript, timeout)
    peer.sendall(b"Q")
    receive_until(peer, b"INPUT-CTRL-NORMAL", transcript, timeout)
    peer.sendall(b"\x05")
    receive_until(peer, b"INPUT-CTRL-RAW", transcript, timeout)
    peer.sendall(b"\x05")
    receive_until(peer, b"INPUT-SEQUENCE", transcript, timeout)
    peer.sendall(b"\x1b[A")
    receive_until(peer, b"INPUT-RAW-SEQUENCE", transcript, timeout)
    peer.sendall(b"\x1b[A")
    receive_until(peer, b"INPUT-LINEFEED", transcript, timeout)
    peer.sendall(b"\nR")
    receive_until(peer, b"INPUT-BURST", transcript, timeout)
    peer.sendall(
        b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-"
    )
    receive_until(peer, b"INPUT-CLEAR", transcript, timeout)
    peer.sendall(b"XYZ")
    receive_until(peer, b"INPUT-UNTIL", transcript, timeout)
    peer.sendall(b"U")
    receive_until(peer, b"INPUT-UNTIL-EXPIRED", transcript, timeout)
    peer.sendall(b"E")
    finish(peer, transcript, b"INPUT-DONE", timeout)
    return transcript


def drive_interactive(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"INTERACTIVE-ANSWER", transcript, timeout)
    peer.sendall(b"xY")
    receive_until(peer, b"INTERACTIVE-STRING", transcript, timeout)
    peer.sendall(b"ab\bCDEFG\r")
    receive_until(peer, b"INTERACTIVE-HIGH", transcript, timeout)
    peer.sendall(b"\x7f\x80\xff\r")
    receive_until(peer, b"INTERACTIVE-EDIT", transcript, timeout)
    peer.sendall(b"ab1c\r")
    receive_until(peer, b"INTERACTIVE-EDIT-CANCEL", transcript, timeout)
    peer.sendall(b"\x1b")
    receive_until(peer, b"INTERACTIVE-MULTILINE", transcript, timeout)
    peer.sendall(b"Hi\rThere\x1a")
    receive_until(peer, b"INTERACTIVE-POPUP-HOTKEY", transcript, timeout)
    peer.sendall(b"s")
    receive_until(peer, b"INTERACTIVE-POPUP-CANCEL", transcript, timeout)
    peer.sendall(b"\x1b")
    receive_until(peer, b"INTERACTIVE-POPUP-KEEP", transcript, timeout)
    peer.sendall(b"l")
    receive_until(peer, b"INTERACTIVE-PAGE-CANCEL", transcript, timeout)
    peer.sendall(b"\r")
    receive_until(peer, b"INTERACTIVE-PAGE-DISABLED", transcript, timeout)
    peer.sendall(b"reason\r\r")
    receive_until(peer, b"INTERACTIVE-CHAT", transcript, timeout)
    peer.sendall(b"A")
    finish(peer, transcript, b"INTERACTIVE-DONE", timeout)
    return transcript


def drive_edit(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"EDIT-FORMATS", transcript, timeout)
    peer.sendall(b"x7x x-!1d1bx/|fxa1Q1jx+1r|w!3xn@\r")
    receive_until(peer, b"EDIT-INSERT", transcript, timeout)
    peer.sendall(b"\x1b[H\x1b[Cb\r")
    receive_until(peer, b"EDIT-OVERWRITE", transcript, timeout)
    peer.sendall(b"\x1b[H\x1b[2~x\r")
    receive_until(peer, b"EDIT-DELETE", transcript, timeout)
    peer.sendall(b"\x1b[H\x1b[3~\x1b[F\x08\r")
    receive_until(peer, b"EDIT-KILL", transcript, timeout)
    peer.sendall(b"\x19x\r")
    receive_until(peer, b"EDIT-AUTO-DELETE", transcript, timeout)
    peer.sendall(b"x\r")
    receive_until(peer, b"EDIT-PREVIOUS", transcript, timeout)
    peer.sendall(b"\x1b[A")
    receive_until(peer, b"EDIT-NEXT", transcript, timeout)
    peer.sendall(b"\t")
    receive_until(peer, b"EDIT-FILL-AUTO", transcript, timeout)
    peer.sendall(b"\rx12")
    receive_until(peer, b"EDIT-PERMALITERAL", transcript, timeout)
    peer.sendall(b"\r")
    receive_until(peer, b"EDIT-STRICT", transcript, timeout)
    peer.sendall(b"\x1b[H\x1b[2~\x1b[3~x\x08\r")
    receive_until(peer, b"EDIT-PASSWORD", transcript, timeout)
    peer.sendall(b"\r")
    receive_until(peer, b"EDIT-MULTILINE-MENU", transcript, timeout)
    peer.sendall(b"AB\tC\x1b")
    receive_until(peer, b"EDIT-MENU-RESUMED", transcript, timeout)
    peer.sendall(b"\x1a")
    receive_until(peer, b"EDIT-MULTILINE-GROW", transcript, timeout)
    peer.sendall(b"Growing text\x1a")
    finish(peer, transcript, b"EDIT-DONE", timeout)
    return transcript


def drive_display(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"DISPLAY-OUTPUT", transcript, timeout)
    receive_until(peer, b"DISPLAY-HOTKEY-WAIT", transcript, timeout)
    receive_until(peer, b"MENU-CONTENT", transcript, timeout)
    peer.sendall(b"2")
    receive_until(peer, b"DISPLAY-HOTKEY-EARLY", transcript, timeout)
    peer.sendall(b"A")
    receive_until(peer, b"DISPLAY-HOTKEY-QUEUED", transcript, timeout)
    peer.sendall(b"Q")
    finish(peer, transcript, b"DISPLAY-DONE", timeout)
    for expected in (
        b"DISP-STRING-RRR-PRINTF-7",
        b"-EMU-",
        b"ANSI-CONTENT",
        b"SECTION-CONTENT",
        b"ITEM.TXT",
        b"MENU-CONTENT",
    ):
        if expected not in transcript:
            raise RuntimeError(
                f"display transcript lacks {expected!r}: {bytes(transcript)!r}"
            )
    return transcript


def drive_emulation(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"EMULATION-PAGING", transcript, timeout)
    receive_until(peer, b"Continue?", transcript, timeout)
    peer.sendall(b"=")
    finish(peer, transcript, b"EMULATION-DONE", timeout)
    for expected in (
        b"EMU-USER|X-X-----|42",
        b"FORMAT-RIP",
        b"FORMAT-AVATAR",
        b"FORMAT-ANSI",
        b"FORMAT-ASCII",
        b"PAGE-LINE-4",
    ):
        if expected not in transcript:
            raise RuntimeError(
                f"emulation transcript lacks {expected!r}: "
                f"{bytes(transcript)!r}"
            )
    return transcript


def drive_listing(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"LISTING-CANCEL", transcript, timeout)
    receive_until(peer, b"Continue?", transcript, timeout)
    peer.sendall(b"n")
    receive_until(peer, b"LISTING-AFTER-CANCEL", transcript, timeout)
    receive_until(peer, b"LISTING-PAGING", transcript, timeout)
    receive_until(peer, b"Continue?", transcript, timeout)
    peer.sendall(b"=")
    finish(peer, transcript, b"LISTING-DONE", timeout)
    for expected in (
        b"ITEM.TXT",
        b"LIST-TITLE",
        b"EXIST.TXT",
        b"MATCH1.DAT",
        b"MATCH2.DAT",
        b"OFFLINE-MARK",
        LONG_DESCRIPTION[:56],
        b"AFTER-LONG-LINE",
        b"COLOR.TXT",
        b"PAGE-LINE-4",
    ):
        if expected not in transcript:
            raise RuntimeError(
                f"listing transcript lacks {expected!r}: "
                f"{bytes(transcript)!r}"
            )
    for unexpected in (b"CANCEL-LINE-3", LONG_DESCRIPTION):
        if unexpected in transcript:
            raise RuntimeError(
                f"listing transcript unexpectedly contains {unexpected!r}: "
                f"{bytes(transcript)!r}"
            )
    return transcript


def drive_popup(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"POPUP-DOWN", transcript, timeout)
    peer.sendall(b"\x1b[B\x1b[B\r")
    receive_until(peer, b"POPUP-UP-WRAP", transcript, timeout)
    peer.sendall(b"\x1b[A\r")
    receive_until(peer, b"POPUP-IGNORED", transcript, timeout)
    peer.sendall(b"\x1b")
    time.sleep(0.3)
    peer.sendall(b"\x1b[C\x1b[B\r")
    receive_until(peer, b"POPUP-HOTKEY", transcript, timeout)
    peer.sendall(b"b")
    receive_until(peer, b"POPUP-LEFT", transcript, timeout)
    peer.sendall(b"\x1b[D")
    receive_until(peer, b"POPUP-RIGHT", transcript, timeout)
    peer.sendall(b"\x1b[C")
    receive_until(peer, b"POPUP-NUMERIC-LEFT", transcript, timeout)
    peer.sendall(b"4")
    receive_until(peer, b"POPUP-NUMERIC-RIGHT", transcript, timeout)
    peer.sendall(b"6")
    receive_until(peer, b"POPUP-KEEP-FIRST", transcript, timeout)
    peer.sendall(b"\x1b[B\r")
    receive_until(peer, b"POPUP-KEEP-RESUME", transcript, timeout)
    peer.sendall(b"\x1b[B\r")
    receive_until(peer, b"POPUP-CANCEL-FIRST", transcript, timeout)
    peer.sendall(b"\r")
    receive_until(peer, b"POPUP-CANCEL-RESUME", transcript, timeout)
    peer.sendall(b"\x1b")
    finish(peer, transcript, b"POPUP-DONE", timeout)
    return transcript


def drive_session(peer: socket.socket, timeout: int) -> bytearray:
    transcript = bytearray()
    receive_until(peer, b"SESSION-AUTODETECT", transcript, timeout)
    receive_until(peer, b"\x1b[6n", transcript, timeout)
    peer.sendall(b"\x1b[1;1R")
    receive_until(peer, b"\x1b[!", transcript, timeout)
    peer.sendall(b"rIp12345678901")
    receive_until(peer, b"SESSION-AUTODETECT-KNOWN", transcript, timeout)
    receive_until(peer, b"SESSION-AUTODETECT-FAIL", transcript, timeout)
    receive_until(peer, b"SESSION-TIMER-ARMED", transcript, timeout)
    receive_until(peer, b"TIME-MESSAGE", transcript, timeout + 3)
    peer.sendall(b"Z")
    receive_until(peer, b"SESSION-DISCONNECT", transcript, timeout)
    peer.shutdown(socket.SHUT_RDWR)
    return transcript


DRIVERS = {
    "input": drive_input,
    "interactive": drive_interactive,
    "edit": drive_edit,
    "display": drive_display,
    "emulation": drive_emulation,
    "listing": drive_listing,
    "popup": drive_popup,
    "session": drive_session,
}


def drive_scenario(peer: socket.socket, scenario: str,
                   timeout: int = 10) -> bytearray:
    try:
        driver = DRIVERS[scenario]
    except KeyError as error:
        raise ValueError(f"unknown transport scenario: {scenario}") from error
    return driver(peer, timeout)
