# Component test backlog

This backlog records useful interaction tests which cannot be expressed
reliably through the installed OpenDoors public API. These are candidates for
a later component-test layer with private interfaces and mocks. They are not
acceptance-test skips, and no production test hooks should be added for them.

| Component and interaction | Proposed controlled boundary | Why public acceptance is insufficient |
| --- | --- | --- |
| Door32 DLL discovery, missing exports, initialization failure, read/write failures, available-input event, and offline event | A component-loaded Door32 substitute implementing each export and deterministic event handles | A substitute DLL is a mock; no real BBS-owned Door32 session can deterministically force every loader and event failure. |
| Win32 serial open, configuration, timeout, event-wait, modem-status, and close failures | Mock Win32 communications calls around `CreateFile`, `Get/SetCommState`, `Get/SetCommTimeouts`, `WaitCommEvent`, and `GetCommModemStatus` | A virtual COM pair covers successful byte flow but cannot portably force individual API calls to fail after a valid open. |
| Socket `poll`/`select`, peek, receive, send, ioctl, and close error branches | Scripted socket-system-call wrapper returning errors and partial operations | A real peer can close or apply backpressure, but inducing exact syscall errors and partial-write boundaries is host- and timing-dependent. |
| Socket transmit saturation and recovery at each internal buffering boundary | Deterministic send-capacity and partial-write wrapper | Refusing to read from a real loopback peer can exercise broad backpressure but cannot guarantee which OpenDoors or kernel buffer saturates. |
| DOS UART interrupt causes, FIFO detection, RX overrun, TX-full, RTS/CTS transitions, and nonstandard IRQ/port layouts | Emulated port-I/O and interrupt-vector component boundary | DOSBox validates end-to-end UART traffic but does not expose deterministic register-level fault and interrupt sequencing to the public door. |
| FOSSIL discovery/version rejection, individual INT 14h failures, status transitions, and block-transfer fallback | Scripted FOSSIL interrupt provider | X00 supplies a successful real FOSSIL path; forcing each interrupt result requires replacing the driver contract. |
| Allocation failure during communications, screen, popup, editor, configuration, and input-queue setup | Allocation wrapper with call-index failure injection | Exhausting process memory is unsafe and nondeterministic, and the public API has no allocator injection interface. |
| Timer rollover, wall-clock jumps, and platform sleep/timer failures | Controllable clock and timer provider | Public tests can wait for ordinary warning/timeout behavior but cannot safely control the system clock or reproduce rollover boundaries. |
| Filesystem partial read/write, flush, seek, and close failures after a successful open | Scripted file-I/O wrapper | Permissions and missing files cover open failures only; later failures are filesystem-specific and cannot be induced portably. |
| Named-reservation crashes during header creation or participant append, torn payloads, lock contention at each byte range, equal-ticket ordering, and stale-participant cleanup | Scripted shared-file and byte-range-lock provider with independently scheduled participants | Real cooperating processes can cover broad successful arbitration, but cannot deterministically stop at every commit boundary or inject every filesystem failure and interleaving. |
| Windows frame message-post, synchronous-send, queue-post, and shutdown delivery failures | Mock window/message/thread primitives | Real GUI acceptance covers successful painting and shutdown, but deterministic failure of each fallback would require intercepting Win32 calls. |

Add entries when acceptance coverage review identifies another worthwhile
interaction whose inputs or observations require a mock. Keep public invalid
parameter checks and failures which can be produced by real files, processes,
peers, or timers in the acceptance suite instead.
