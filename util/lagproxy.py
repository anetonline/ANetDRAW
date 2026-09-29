#!/usr/bin/env python3
"""Telnet timing proxy: sits between SyncTERM and the BBS on the CLIENT
machine and timestamps every chunk in both directions, to see where
drawing lag comes from.

    python3 util/lagproxy.py bbs.a-net.fyi 23          # listens on localhost:2323
    syncterm telnet://localhost:2323                    # connect through it
    ... log in, open ANetDRAW, draw a stroke, stop, wait, quit ...
    Ctrl+C the proxy -> prints a summary; full log in /tmp/anetdraw_lagproxy.log

For every mouse report SyncTERM sends up, the summary shows how long the
BBS took to send the next screen update back, and how long after the
LAST mouse report the BBS kept sending (the tail you see as lag).
"""
import asyncio
import re
import sys
import time

HOST = sys.argv[1] if len(sys.argv) > 1 else "bbs.a-net.fyi"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 23
LISTEN = int(sys.argv[3]) if len(sys.argv) > 3 else 2323
LOG = "/tmp/anetdraw_lagproxy.log"
MOUSE = re.compile(rb"\x1b\[<\d+;\d+;\d+[Mm]")

events = []  # (time, "up"/"down", nbytes, n_mouse_reports)
logf = open(LOG, "w")


def note(direction, data):
    t = time.time()
    n_mouse = len(MOUSE.findall(data)) if direction == "up" else 0
    events.append((t, direction, len(data), n_mouse))
    logf.write("%.4f %-4s %5d mouse=%d %r\n" % (t, direction, len(data), n_mouse, data[:60]))
    logf.flush()


async def pipe(reader, writer, direction):
    try:
        while True:
            data = await reader.read(65536)
            if not data:
                break
            note(direction, data)
            writer.write(data)
            await writer.drain()
    except (ConnectionError, asyncio.CancelledError):
        pass
    finally:
        writer.close()


async def handle(cr, cw):
    sr, sw = await asyncio.open_connection(HOST, PORT)
    print("connected to %s:%d -- logging to %s" % (HOST, PORT, LOG), flush=True)
    await asyncio.gather(pipe(cr, sw, "up"), pipe(sr, cw, "down"))
    # the caller disconnected: report right away (no Ctrl+C needed)
    summary()
    events.clear()


def summary():
    ups = [e for e in events if e[1] == "up" and e[3] > 0]
    downs = [e for e in events if e[1] == "down"]
    if not ups:
        print("no mouse reports seen")
        return
    waits = []
    j = 0
    for t, _, _, _ in ups:
        while j < len(downs) and downs[j][0] < t:
            j += 1
        if j < len(downs):
            waits.append((downs[j][0] - t) * 1000)
    waits.sort()
    last_up = ups[-1][0]
    after = [d for d in downs if d[0] > last_up]
    tail = (after[-1][0] - last_up) * 1000 if after else 0
    print("\n=== session summary ===")
    print("mouse reports sent: %d, chunks received: %d" % (sum(e[3] for e in ups), len(downs)))
    print("report -> next update from BBS: median %.1f ms, 90%% %.1f ms, max %.1f ms"
          % (waits[len(waits) // 2], waits[int(len(waits) * 0.9)], waits[-1]))
    print("BBS kept sending for %.0f ms after the last mouse report (%d chunks, %d bytes)"
          % (tail, len(after), sum(d[2] for d in after)), flush=True)


async def main():
    srv = await asyncio.start_server(handle, "127.0.0.1", LISTEN)
    print("listening on localhost:%d -> %s:%d  (Ctrl+C when done)" % (LISTEN, HOST, PORT))
    async with srv:
        await srv.serve_forever()


try:
    asyncio.run(main())
except KeyboardInterrupt:
    pass
summary()
