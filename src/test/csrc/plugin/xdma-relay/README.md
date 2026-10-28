# Remote XDMA relay

The optional relay runs XDMA device I/O on the board machine and the existing
FPGA host, packet parser and REF workers on a compute machine. It uses the same
packet pool and fork REF implementation as local XDMA. No NEMU or XDMA driver
changes are required. Leave XDMA_RELAY_* variables unset for local I/O.

## Layout

This plugin owns the relay client, wire protocol, board daemon, build rules
and documentation. The FPGA Makefile includes `relay.mk` to link the client;
the existing XDMA transport keeps only I/O dispatch and packet assembly hooks.
Local XDMA remains the default unless the relay environment is configured.

## Build and start

Build fpga-host normally. Build the board executable on the board machine with
make fpga-xdma-relay. Create a private, per-run token file on each machine with
the same random value (32 to 128 bytes; trailing newlines are ignored). Keep the
files readable only by the run owner. Do not put the token in arguments or logs.

On the compute machine, export these before starting fpga-host:

    export XDMA_RELAY_LISTEN=192.0.2.10
    export XDMA_RELAY_PORT=55273
    export XDMA_RELAY_TOKEN_FILE=/private/run/relay.token
    ./build/fpga-host <normal FPGA host arguments>

On the board machine:

    ./build/fpga-xdma-relay --connect 192.0.2.10 --port 55273 \
      --token-file /private/run/relay.token --zstd

The compute machine listens and the board connects. Either command may start
first: the listener and daemon each allow 120 seconds to establish the session.
--connect-timeout SECONDS changes the daemon's initial connection deadline.
An explicit IPv4 bind address is required. The daemon defaults to /dev/xdma0;
--device-prefix PATH selects the device family. It runs in the foreground,
serves one session and exits on disconnection or SIGINT/SIGTERM. It never silently
reconnects a broken session. env-scripts can own SSH launch, token files and
cleanup; this executable itself does not manage SSH or UART.

## Transport and failure semantics

A versioned token handshake authenticates the control connection and each C2H
channel. The compute host supplies its generated packet size and channel count;
the daemon does not assume a CPU profile. C2H reads start only when the host
requests the stream, after workload initialization. The daemon keeps a bounded
1 MiB queue per channel and batches at most 64 KiB per network frame, flushing
approximately every millisecond. Optional --zstd uses level 1 and sends
incompressible blocks raw. Build the host with its usual zstd support for this
option; a host built with NO_ZSTD_COMPRESSION supports raw frames only.

Short reads preserve byte order; the host accumulator publishes only complete
packets. Driver errors, including ETIMEDOUT, are forwarded as errors instead of
being converted to successful reads. Whether a timeout can be retried during
fork checker drain remains the host's policy. The relay does not extend that
policy or synthesize packets. EOF and broken/truncated frames cannot complete
a packet successfully. Socket I/O has a 300-second inactivity timeout; host
shutdown also shuts down its sockets to wake receiver threads immediately.

Control RPCs perform exactly one requested 32-bit MMIO read/write, avoiding
adjacent registers with possible side effects. H2C writes use aligned buffers
and bounded 1 MiB requests. Bypass access is unsupported. Frame lengths and
profile sizes are bounded; invalid versions or authentication fail closed.
Forked checkers inherit descriptors but only the supervisor owns socket shutdown;
REF children continue reading the shared packet pool, not TCP streams.

Use a trusted, isolated network or an external encrypted tunnel. Tokens provide
session authentication, not encryption. The board machine must retain exclusive
ownership of XDMA devices, reset controls and UART for the run. End-to-end board
validation is separate from local protocol checks.
