# Local patches to the vendored OpenDoors

Base: upstream commit recorded in `OPENDOORS_COMMIT`. Every change below is
marked `ANetDRAW local patch` in the source. Re-apply after any OpenDoors
update, and check whether upstream has fixed it first.

## 1. `src/ODGetIn.c` — escape-sequence buffer overflow (2026-09-28)

`ODGetInputCore()` builds up remote input in the 10-byte static
`szCurrentSequence` while it decides whether the bytes form a key sequence
(arrows, F-keys, ...). After an ESC, it kept appending **every** byte that
arrived within the 250 ms latency window, with no length check, and it didn't
stop even when the bytes could no longer match any sequence. A caller sending
ESC followed by a quick burst of 9 or more bytes (typing fast, pasting, or
doing it on purpose) wrote past the end of the buffer into the globals after
it. The write kept going for as long as the burst lasted, so a remote caller
could corrupt memory.

Found by AddressSanitizer while fuzzing ANetDRAW with random keystrokes
(`tests/fuzz_keys.py`). Reproduced with ESC followed by 20 bytes in one burst.

Fix:
- The accumulation loop only runs while `nSequenceLen < SEQUENCE_BUFFER_SIZE - 1`.
  The check sits in the loop condition, before the next byte is read, so no
  keystroke is lost.
- The loop stops as soon as the buffer is no longer the start of any known
  sequence (`!ODHaveStartOfSequence()`). The existing code after the loop then
  hands the buffered bytes back one key at a time.

Real sequences decode exactly as before (the phase 1-3 session tests
exercise arrows, Home/End, PgUp/PgDn, Insert/Delete, F1-F12 in both
`ESC [ n ~` and `ESC O x` forms, and the lone-ESC timeout).

What else is affected: anything that uses `od_get_input()`, which includes
OpenDoors' own `od_edit_str()`, `od_popup_menu()`, and the multiline editor.
This should be reported upstream (RealDeuce/OpenDoors).

**Upstream status (checked 2026-09-28):** RealDeuce/OpenDoors master still
has the same unbounded append (`ODGetIn.c` line 436), so this hasn't been
fixed upstream yet.

## 2. `src/ODInEx1.c` — BBSDEV.DRP socket sessions treated as local (2026-09-28)

The vendored BBSDEV.DRP reader (added in upstream commit 4b4ace7, just before
our base) never sets `od_control.baud`, and the format has no baud-rate line.
With `baud` left at 0, OpenDoors treats the session as local:
`od_carrier()` reports no caller and nothing gets sent to the socket. A
BBSDEV.DRP caller saw a blank screen, and the door exited right away because
it believed the caller had hung up. Found with `tests/screen_size_test.py`.

Fix: after the communications-type block, set `od_control.baud = 1L` on
POSIX, and on other platforms `0L` for a local session or `1L` otherwise.
These are the same values upstream's later rewrite of the BBSDEV.DRP reader
sets, so this backports upstream's own behavior rather than inventing new
behavior. Dropping this patch is safe if the vendored copy is ever updated to
that rewrite.

## 3. `src/ODGetIn.c` — 1 ms socket poll per character (mouse lag) (2026-09-28)

`ODGetInputCore()` calls `od_kernel()` at several points just to pick up
bytes that may have arrived. On POSIX the kernel's socket read waits up to
`ODMaxMSToWait` (default 1 ms) in `poll()`, so each of those calls blocked
about 1.4 ms, even when every byte the caller needed was already queued.
It costs about two calls per input character. A typed key barely notices
(about 2 ms), but a 12-byte SGR mouse report took about 29 ms. A real mouse
drag sends 60-100 reports a second, so the door fell further and further
behind: callers drew a shape and saw it appear seconds later.

Found by tracing the door's system calls for a single mouse report:
every byte had been read within 0.1 ms, followed by about 21 one-millisecond
`poll()` timeouts before the reply went out.

Fix: those four kernel calls go through `ODGetInKernelPoll()`, which sets
`ODMaxMSToWait` to 0 around the call. The real waits in `ODGetInputWait()`
and `ODInQueueGetNextEvent()` still set their own `ODMaxMSToWait`, so an
idle door still blocks in `poll()` and doesn't spin (idle CPU measured at 0%).

Measured with `tests/latency_test.py`, against a build with only this patch
reverted and against the patched build: a single mouse event took 25-26 ms
before the fix and 0.2 ms after. An 80 Hz drag stream of 150 events left the
screen 600+ ms behind before the fix and 0 ms behind after. The test fails on
the reverted build and passes on the fixed one.
