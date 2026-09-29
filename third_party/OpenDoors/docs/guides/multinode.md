# Multi-node operation

Several copies of a door may run at the same time. OpenDoors reports the
current node through
[`od_control.od_node`](../reference/control/connection.md#od_node), but the
application remains responsible for choosing node-specific temporary files and
protecting shared application data.

## Named reservations

OpenDoors provides portable, cooperative filesystem-backed reservations for
resources shared by DOS, Windows, and Unix nodes. Configure one registry before
initialization:

```c
if(!od_reserve_configure("MYDOOR.SYN"))
    return 1;
od_init();
```

Every cooperating process must use the same registry path. At initialization,
the process finds or appends a fixed-size record for its `NodeX` identity, or
for the nodeless `Local` identity, and holds that record's lifetime lock. A
later session with the same identity reuses its committed record. Because two
processes may not legitimately use the same identity, failure to acquire that
record's lifetime lock is a fatal initialization error rather than a reason to
append a duplicate record.

Use a reservation name for each independently protected resource:

```c
if(od_reserve_request("Users")
    && od_reserve_wait(20000) == OD_RESERVE_ACQUIRED)
{
    FILE *users = fopen("USERS.DAT", "r+b");
    if(users != NULL)
    {
        /* Read the current record, update it, flush it, then close it. */
        fclose(users);
    }
    od_reserve_end();
}
```

Requests with the same case-sensitive name are ordered using a bakery ticket.
Different names do not block one another. Equal ticket values are expected and
are ordered by the participants' stable record indices. A participant does not
rewrite its ticket while that request remains queued. A timeout leaves the
request queued, so call
[`od_reserve_end()`](../reference/api/od_reserve_end.md) to cancel it.

The registry format is fixed-size and little-endian. Its header is immutable
after creation. Each live participant writes only its own record payload and
read-locks peer payloads; the registry lock serializes append and record-count
snapshots. A new identity is allocated at physical end-of-file, while a later
session of the same identity reclaims its previous committed record. An
abandoned or partially written append slot becomes a permanent tombstone and
is not adopted by another identity.

These guarantees are cooperative. All programs which access the protected
application resource must request the same reservation name. The shared
filesystem must propagate byte-range locks and file writes between hosts. On
DOS, multi-node operation also requires a redirector or `SHARE.EXE`
implementation which supports byte-range locking. If DOS reports that the
record-locking function does not exist, OpenDoors treats reservations as
uncontended so the same program still works on a non-shared, single-node DOS
system. That fallback does not provide coordination with another process.

See [`od_reserve_configure()`](../reference/api/od_reserve_configure.md),
[`od_reserve_request()`](../reference/api/od_reserve_request.md), and
[`od_reserve_wait()`](../reference/api/od_reserve_wait.md) for the complete
contracts.

## Protect the complete update

Acquire the reservation before reading the data that will be modified. Keep it
through the write, flush, and close. Reading first and reserving only for the
write still permits two nodes to calculate updates from the same stale value.

Keep reserved intervals short. Do not wait for user input while holding one,
and avoid unrelated display or file work. The `ex_vote.c` example uses the
`VoteUsers` and `VoteQuestions` names so updates to the two files remain
independent.

## Node-specific configuration

Node numbers remain useful for log names, work directories, temporary protocol
files, and locating per-node door-information files. They are not a substitute
for reserving shared state.

If each node has a separate door-information directory, pass the appropriate
path through the BBS command line or a per-node configuration. If all nodes
share one OpenDoors configuration file, avoid putting node-specific connection
settings in that shared file. The BBS, `TASK` environment variable, or
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md) should supply the
node identity instead.
