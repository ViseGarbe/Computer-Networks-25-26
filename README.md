# Computer Networks 25-26

Collection of C exercises and small protocol prototypes developed while studying computer networks, low-level systems programming, and data representation.

The directory mixes:

- introductory C memory/layout exercises
- raw-socket networking experiments
- simple HTTP client/server examples
- sender-side simulations of Go-Back-N and Selective Repeat

This is primarily a study/workbench repository, so the code is educational and exploratory rather than packaged as a single polished project.

## Repository Layout

### Introductory C / memory representation

- `25-02.c`: pointers, memory addresses, pointer arithmetic, and byte-level inspection
- `26-02-A.c`: recursion, stack growth, and a small dynamic-allocation example
- `26-02-B.c`: alignment and layout of global variables with different types
- `04-03.c`: IEEE 754 float dissection with bit operations
- `Endiannes.c`: endianness detection and byte-order visualization
- `C/Structures/struct.c`, `C/Structures/structDate.c`: small structure-related practice files

### Protocol and systems exercises

- `env.c`: signal-driven I/O and timer experiment using `SIGIO`, `SIGALRM`, and `poll`
- `gbn.c`: sender-side Go-Back-N sliding-window demo
- `sr.c`: sender-side Selective Repeat sliding-window demo with per-packet ACK state and timers
- `29-04.c`: partial TCP control block / transmission-buffer sketch - INCOMPLETE

### Raw-socket networking tools

- `sniff.c`: packet sniffer based on `AF_PACKET`
- `arp.c`: ARP request/reply experiment to resolve MAC addresses
- `ping.c`: handcrafted Ethernet/IP/ICMP ping prototype
- `traceroute.c`: traceroute-style experiment built on the same low-level approach

### TCP / HTTP examples

- `webserver.c`: minimal TCP HTTP-like server
- `webclient.c`: minimal TCP client for local testing
- `test.html`: sample file to serve during HTTP tests

### Generated binaries currently in the directory

- `04-03`
- `25-02`
- `26-02-A`
- `26-02-B`
- `Endiannes`

## Requirements

The repository is easiest to use on Linux for the raw-socket programs.

- C compiler such as `gcc` or `clang`
- POSIX environment
- Linux headers for `AF_PACKET` examples such as `<linux/if_packet.h>`
- root privileges or `CAP_NET_RAW` for raw-socket programs (`sniff.c`, `arp.c`, `ping.c`, `traceroute.c`)

Notes:

- Several networking files assume a Linux-style environment and may not compile on macOS as-is.
- Some examples hard-code interface names, IP addresses, MAC addresses, ports, or network parameters. Review and adapt them before running.

## Build

There is no global `Makefile` yet. Compile files individually.

Examples:

```bash
gcc ex_name.c -o ex_name
```

## Running Selected Programs

### `webserver.c` and `webclient.c`

Start the server in one terminal:

```bash
./webserver
```

Then run the client in another:

```bash
./webclient
```

`webclient.c` is configured for localhost testing, while `webserver.c` currently binds to port `80`. Depending on your system, binding to port `80` may require elevated privileges.

### `gbn.c` and `sr.c`

These programs are interactive sender-side simulations of sliding-window protocols.

- they automatically send packets until the sender window is full
- ACK values are read from standard input
- timers trigger retransmissions

Example:

```bash
./sr
```

Then type sequence numbers such as:

```text
1
0
4
```

In `sr.c`, ACKs can arrive out of order; the window advances only when the first unconfirmed packet is eventually acknowledged.

## Caveats

- Many files are written as lab notes or in-progress experiments.
- Signal-driven examples (`env.c`, `gbn.c`, `sr.c`) are useful for understanding the mechanisms, but they are not production-style event-loop implementations.
- Some files contain hard-coded addressing values that should be customized before use on another machine or network.