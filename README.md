# 🌐 NetScope
### NORMANTOYS // LINUX NETWORK VISIBILITY

> OBSERVE. MEASURE. UNDERSTAND.

NetScope is a lightweight Linux network monitor written in modern C++.

It reads kernel-exposed telemetry from `/proc/net/dev` and `/proc/net/tcp*`, calculates per-interface traffic rates, and exposes TCP socket state directly from the terminal.

No packet capture. No privileged sniffer. No third-party networking framework. Just C++ talking to Linux.

## Why it exists

NetScope is the second NORMANTOYS systems-security project, focused on the layer underneath a SOC dashboard: knowing what the host's network stack is actually doing.

```text
/proc/net/dev   ──> interface counters ──> RX/TX rate
/proc/net/tcp*  ──> socket table       ──> TCP state
                                      ↓
                              NORMANTOYS // NETSCOPE
```

## Features

- Per-interface RX/TX bandwidth calculation
- Continuous terminal refresh
- TCP connection table
- IPv4 and IPv6 TCP socket decoding
- Linux-only `/proc` telemetry model
- Security-Lab compatible NDJSON export
- Collision-resistant per-process event IDs
- Microsecond-resolution event timestamps
- Strict CLI interval validation
- No root requirement for the included telemetry
- CMake build + CTest unit tests
- GitHub Actions CI

## Requirements

- Linux
- C++17 compiler
- CMake 3.16+

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
```

## Run

One snapshot:

```bash
./build/netscope --once
```

Continuous monitoring every second:

```bash
./build/netscope
```

Two-second sampling with TCP connections:

```bash
./build/netscope --interval 2 --connections
```

Security-Lab NDJSON export:

```bash
./build/netscope --once --json --connections > events.ndjson
```

The `--json` mode emits one JSON object per line using the shared Security-Lab event contract.

## Example

```text
┌──────────────────────────────────────────────────────────────┐
│ NORMANTOYS // NETSCOPE                                      │
│ Linux network telemetry                                     │
└──────────────────────────────────────────────────────────────┘

INTERFACE                    RX              TX
------------------------------------------------
lo                         0 bps           0 bps
enp3s0                    24 Kbps         13 Kbps

TCP CONNECTIONS
LOCAL                REMOTE               STATE
----------------------------------------------------------
0.0.0.0:22           0.0.0.0:0           LISTEN
192.168.1.20:42810   142.250.72.14:443    ESTABLISHED
```

## Testing

```bash
ctest --test-dir build --output-on-failure
```

The suite covers `/proc/net/dev`, IPv4/IPv6 TCP parsing, event ID uniqueness, malformed TCP ports, and invalid CLI interval input.

## Architecture

See [`docs/architecture.md`](docs/architecture.md) for the data flow and sampling model.

## Roadmap

- [ ] Interface discovery metadata (MAC, MTU, state)
- [ ] Top talkers by remote endpoint
- [ ] Threshold alerts for traffic spikes
- [ ] UDP socket view
- [ ] eBPF-backed telemetry mode

## NORMANTOYS

This project is part of a personal systems and cybersecurity portfolio under the NORMANTOYS identity.

**BUILD. BREAK. UNDERSTAND.**

## License

MIT
