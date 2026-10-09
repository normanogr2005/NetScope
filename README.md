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
- Interface MAC address, MTU, and operational-state metadata from sysfs
- Configurable RX/TX traffic threshold warnings
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

Warn when either RX or TX reaches 100 Mbps:

```bash
./build/netscope --interval 1 --alert-mbps 100
```

The threshold is optional. Without `--alert-mbps`, threshold warnings are disabled.
In NDJSON mode, events above the configured threshold are emitted with `severity: "warning"`.

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

The suite covers `/proc/net/dev` parsing, RX/TX rate calculations, invalid elapsed times, counter resets, sysfs interface metadata, IPv4/IPv6 TCP decoding, malformed TCP endpoints, event ID uniqueness, and invalid CLI input. Assertions remain enabled in Release builds so CI checks expected results. CI also runs AddressSanitizer and UndefinedBehaviorSanitizer.

## Claude repository audit

The repository includes [CLAUDE.md](CLAUDE.md) and a [security audit checklist](docs/security-audit-checklist.md). To run a full read-only Claude audit, add an `ANTHROPIC_API_KEY` repository Actions secret, then open **Actions → Claude Repository Audit → Run workflow**. The workflow has read-only repository permissions and is manually triggered to avoid unexpected API usage. Review the generated workflow summary; an AI audit is an additional review, not proof that the project is vulnerability-free.

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
