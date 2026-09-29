#!/bin/sh
# Run ANetDRAW locally inside SyncTERM -- no BBS, no network -- for
# testing drawing and mouse behavior on this machine's own display.
#   ./util/syncterm_local.sh            (uses build/anetdraw)
# The door's input/output timing log goes to /tmp/anetdraw_local_trace.log.
# SyncTERM's shell: URL has a short length limit, so the door is started
# through a tiny helper with a short relative path.
cd "$(dirname "$0")/.." || exit 1
cat > .door_local.sh <<'INNER'
#!/bin/sh
# -L output must stay CP437 for SyncTERM: under a UTF-8 locale OpenDoors
# converts it to UTF-8 (see ODInEx1.c), which SyncTERM shows as garbage.
export LC_ALL=C
exec ./build/anetdraw -L --trace /tmp/anetdraw_local_trace.log
INNER
chmod +x .door_local.sh
exec syncterm "shell:./.door_local.sh"
